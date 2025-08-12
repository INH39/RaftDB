
#include "raft_node.h"
#include "raft_cluster.h"
#include <cstdint>
#include <string>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <random>
#include <vector>
#include <unordered_map>
#include <thread>
#include <mutex>
#include "raft.pb.h"


void RaftNode::runElectionTimerLoop() {
    while(this->running) {
        std::unique_lock<std::mutex> lock(this->stateMutex);

        if(this->state == NodeState::LEADER){
            lock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(this->electionTimer));
            continue;
        }

        auto now = std::chrono::steady_clock::now();
        auto electionTimeout = this->getRandomElectionTimeout();

        if(now - this->lastHeartbeat >= electionTimeout) {
            std::cout << "Node " << this->nodeId << " hit timeout, starting election" << std::endl;
            this->startElection();
        }

        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(this->electionTimer));
    }
}

void RaftNode::runHeartbeatLoop() {
    while(this->running) {
        std::unique_lock<std::mutex> lock(this->stateMutex);

        if(this->state == NodeState::LEADER) {
            std::cout << "Sending heartbeat ... " << std::endl;
            this->sendHeartbeats();
        }

        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(this->heartbeatInterval));
    }
}

void RaftNode::resetElectionTimer(){
    this->lastHeartbeat = std::chrono::steady_clock::now();
}

std::chrono::milliseconds RaftNode::getRandomElectionTimeout(){
    std::uniform_int_distribution<int> dist(this->electionTimeoutMin.count(), this->electionTimeoutMax.count());
    return std::chrono::milliseconds(dist(this->randomGenerator));
}

void RaftNode::startElection(){
    if(!this->clusterPtr) {
        std::cout << "Node " << this->nodeId << " not part of a cluster, can't start an election" << std::endl;
        return;
    }

    this->becomeCandidate();
    this->resetElectionTimer();
    std::cout << "Node " << this->nodeId << " starting election for term " << this->currentTerm << std::endl;

    int votes = 1;
    std::unordered_map<uint64_t, std::string> nodeAddresses = this->clusterPtr->getNodeAddresses();

    // send request vote rpcs to other nodes
    for(const auto& [otherNodeId, address] : nodeAddresses) {
        if(otherNodeId == this->nodeId) continue;

        Raft::RequestVoteRequest request;
        request.set_term(this->currentTerm);
        request.set_candidateid(this->nodeId);
        request.set_lastlogindex(this->log.size());

        std::uint64_t lastLogTerm = 0;
        if(this->log.size() > 0) {
            auto lastEntry = this->log.getEntry(this->log.size());
            if(lastEntry.has_value()) {
                lastLogTerm = lastEntry.value().term;
            }
        }
        request.set_lastlogterm(lastLogTerm);

        auto response = this->clusterPtr->sendRequestVote(this->nodeId, otherNodeId, request); 
        if(response.term() > this->currentTerm) {
            std::cout << "Node " << this->nodeId << " found higher term " << response.term() 
                        << " from Node " << otherNodeId << ", becoming follower" << std::endl;
            this->becomeFollower(response.term());
            return; 
        }

        if(response.votegranted()){
            votes++;
            std::cout << "Node " << this->nodeId << " received vote from Node " << otherNodeId << std::endl;

        }
    }
    
    // check for majority
    int totalNodes = nodeAddresses.size();
    int majorityVotes = (totalNodes / 2) + 1;

    if(votes >= majorityVotes) {
        std::cout << "Node " << this->nodeId << " won election with " << votes 
                  << "/" << totalNodes << " votes, becoming leader" << std::endl;
        this->becomeLeader();
    }
    else {
        std::cout << "Node " << this->nodeId << " lost election with " << votes 
                  << "/" << totalNodes << " votes" << std::endl;
    }
}

void RaftNode::sendHeartbeats(){
    if(!this->clusterPtr) {
        std::cout << "Node " << this->nodeId << " not part of a cluster, can't send heartbeats" << std::endl;
        return;
    }

    std::unordered_map<uint64_t, std::string> nodeAddresses = this->clusterPtr->getNodeAddresses();
    
    // send heartbeat/log replication rpcs to other nodes
    for(const auto& [otherNodeId, address] : nodeAddresses) {
        if(otherNodeId == this->nodeId) continue;

        // get next idx for this follower (ensure arrays are properly sized)
        if(this->nextIndex.size() <= otherNodeId) {
            std::cout << "Leader (node) " << this->nodeId << " nextIndex array too small for node " << otherNodeId << std::endl;
            continue;
        }
        
        std::uint64_t nextIdx = this->nextIndex[otherNodeId];
        std::uint64_t prevLogIndex = (nextIdx > 1) ? nextIdx - 1 : 0;
        std::uint64_t prevLogTerm = 0;
        
        // get previous log term
        if(prevLogIndex > 0) {
            auto prevEntry = this->log.getEntry(prevLogIndex);
            
            if(prevEntry.has_value()) {
                prevLogTerm = prevEntry.value().term;
            }
        }

        Raft::AppendEntriesRequest request;
        request.set_term(this->currentTerm);
        request.set_leaderid(this->nodeId);
        request.set_prevlogindex(prevLogIndex);
        request.set_prevlogterm(prevLogTerm);
        request.set_leadercommit(this->commitIndex);
        
        // add entries starting from nextIndex
        for(std::uint64_t i = nextIdx; i <= this->log.size(); i++) {
            auto entry = this->log.getEntry(i);
            if(entry.has_value()) {
                Raft::LogEntry* logEntry = request.add_entries();
                logEntry->set_term(entry.value().term);
                logEntry->set_index(entry.value().index);
                logEntry->set_operation(entry.value().operation == OpType::PUT ? "PUT" : "DELETE");
                logEntry->set_key(entry.value().key);
                logEntry->set_value(entry.value().value);
            }
        }
        auto response = this->clusterPtr->sendAppendEntries(this->nodeId, otherNodeId, request); 
        
        if(response.term() > this->currentTerm) {
            std::cout << "Node " << this->nodeId << " found higher term " << response.term() 
                        << " from Node " << otherNodeId << ", steps down by becoming follower" << std::endl;
            this->becomeFollower(response.term());
            return;
        }
        
        if(response.success()) {
            // update next idx and match idx
            std::uint64_t lastSentIndex = prevLogIndex + request.entries().size();
            this->nextIndex[otherNodeId] = lastSentIndex + 1;
            this->matchIndex[otherNodeId] = lastSentIndex;
            
            if(request.entries().size() > 0) {
                std::cout << "Leader (node) " << this->nodeId << " successfully replicated " 
                          << request.entries().size() << " entries to Node " << otherNodeId << std::endl;
            }
        } 
        else {
            // decrement nextIndex and retry next time
            if(this->nextIndex[otherNodeId] > 1) {
                this->nextIndex[otherNodeId]--;
                std::cout << "Leader (node) " << this->nodeId << " decrementing nextIndex to " 
                          << this->nextIndex[otherNodeId] << " for Node " << otherNodeId << std::endl;
            }
        }
    }

    // after heartbeats, advance commit index
    this->advanceCommitIndex();
}

void RaftNode::advanceCommitIndex() {
    if(!this->clusterPtr) return;
    
    std::unordered_map<std::uint64_t, std::string> nodeAddresses = this->clusterPtr->getNodeAddresses();
    std::uint64_t totalNodes = nodeAddresses.size();
    std::uint64_t majorityCount = (totalNodes / 2) + 1;
    
    // find highest log idx that has been replicated to a majority
    for(std::uint64_t index = this->log.size(); index > this->commitIndex; index--) {
        std::uint64_t replicatedCount = 1; // already have leader with this idx
        
        // count followers with this idx
        for(const auto& [otherNodeId, address] : nodeAddresses) {
            if(otherNodeId == this->nodeId) continue;
            
            if(this->matchIndex.size() > otherNodeId && this->matchIndex[otherNodeId] >= index) {
                replicatedCount++;
            }
        }
        
        // majority have this idx + current term -> commit idx
        if(replicatedCount >= majorityCount) {
            auto entry = this->log.getEntry(index);

            if(entry.has_value() && entry.value().term == this->currentTerm) {
                std::uint64_t oldCommitIndex = this->commitIndex;
                this->commitIndex = index;
                
                std::cout << "Leader (node) " << this->nodeId << " advanced commitIndex from " 
                          << oldCommitIndex << " to " << this->commitIndex << std::endl;
                
                // commit entries up to commit idx
                for(std::uint64_t i = oldCommitIndex + 1; i <= this->commitIndex; i++) {
                    auto commitEntry = this->log.getEntry(i);

                    if(commitEntry.has_value()) {
                        this->stateMachine.applyEntry(commitEntry.value());

                        std::cout << " Applied entry " << i << " to state machine of Leader (node) " << this->nodeId << std::endl;
                    }
                }
                
                break;
            }
        }
    }
}

void RaftNode::initializeLeaderState() {
    if(!this->clusterPtr) return;
    
    std::unordered_map<std::uint64_t, std::string> nodeAddresses = this->clusterPtr->getNodeAddresses();
    std::uint64_t nextIdx = this->log.size() + 1; // + 1 because 1-indexed
    
    // clear and resize arrays 
    this->nextIndex.clear();
    this->matchIndex.clear();
    this->nextIndex.resize(nodeAddresses.size() + 1, nextIdx);
    this->matchIndex.resize(nodeAddresses.size() + 1, 0);
    
    std::cout << "Leader (node) " << this->nodeId << " initialized state: nextIndex=" 
              << nextIdx << " for all followers" << std::endl;
}

RaftNode::RaftNode(std::uint64_t nodeId, const std::string& logPath)
        : nodeId(nodeId), log(logPath), currentTerm(0), votedFor(0), commitIndex(0), 
            state(NodeState::FOLLOWER), clusterPtr(nullptr), heartbeatInterval(50),
            electionTimer(10), electionTimeoutMin(150), electionTimeoutMax(300),
            randomGenerator(std::random_device{}())
{
    for(int i = 1; i <= this->log.size(); i++) {
        auto entry = this->log.getEntry(i);
        if(entry.has_value()){
            this->stateMachine.applyEntry(entry.value());
        }
    }
    std::cout << "Node " << this->nodeId << " created with " << this->log.size() << " log entries applied" << std::endl;

    this->resetElectionTimer();
}

RaftNode::~RaftNode() {
    this->stopNode();
}

void RaftNode::becomeFollower(std::uint64_t term){
    this->currentTerm = term;
    this->votedFor = 0;
    this->state = NodeState::FOLLOWER;
    this->resetElectionTimer();
}

void RaftNode::becomeCandidate() {
    this->currentTerm += 1;
    this->votedFor = this->nodeId;
    this->state = NodeState::CANDIDATE;
    this->resetElectionTimer();
}

void RaftNode::becomeLeader() {
    this->state = NodeState::LEADER;
    this->initializeLeaderState();
}

NodeState RaftNode::getState() const {
    return this->state;
}

std::uint64_t RaftNode::getCurrentTerm() const {
    return this->currentTerm;
}

std::uint64_t RaftNode::getNodeId() const {
    return this->nodeId;
}

std::uint64_t RaftNode::getCommitIndex() const {
    return this->commitIndex;
}

RaftCluster* RaftNode::getNodeCluster() {
    return this->clusterPtr;
}

StateMachine* RaftNode::getStateMachine() {
    return &this->stateMachine;
}

Log* RaftNode::getLog() {
    return &this->log;
}

Raft::RequestVoteResponse RaftNode::handleRequestVote(const Raft::RequestVoteRequest& req){
    Raft::RequestVoteResponse response;
    response.set_term(this->currentTerm);
    response.set_votegranted(false);

    // stale request
    if(req.term() < this->currentTerm){
        return response;
    }

    // become follower and update
    if(req.term() > this->currentTerm){
        this->becomeFollower(req.term());
        response.set_term(this->currentTerm);
    }
    
    if(this->votedFor == 0 || this->votedFor == req.candidateid()){
        // check if candidate log is more up to date
        std::uint64_t thisLastIndex = this->log.size();
        std::uint64_t thisLastTerm = 0;
        if(thisLastIndex > 0){
            auto lastEntry = this->log.getEntry(thisLastIndex);
            if(lastEntry.has_value()){
                thisLastTerm = lastEntry.value().term;
            }
        }
        bool logUpToDate  = (req.lastlogterm() > thisLastTerm) || (req.lastlogterm() == thisLastTerm && req.lastlogindex() >= thisLastIndex);

        if(logUpToDate){
            this->votedFor = req.candidateid();
            response.set_votegranted(true);
            std::cout << "Node " << this->nodeId << " grants vote to " << req.candidateid() << " for term " << req.term() << std::endl;
        }
    }
   
    return response;
}

Raft::AppendEntriesResponse RaftNode::handleAppendEntries(const Raft::AppendEntriesRequest& req){
    Raft::AppendEntriesResponse response;
    response.set_term(this->currentTerm);
    response.set_success(false);

    // stale leader
    if(req.term() < this->currentTerm){
        return response;
    }

    // reset timer on valid heartbeat
    if(req.term() >= this->currentTerm) {
        this->resetElectionTimer();
    }

    // become follower and update
    if(req.term() > this->currentTerm){
        this->becomeFollower(req.term());
        response.set_term(this->currentTerm);
    }

    // log consistency
    if(req.prevlogindex() > 0){
        auto prevEntry = this->log.getEntry(req.prevlogindex());

        if(!prevEntry.has_value()){
            std::cout << "Node " << this->nodeId << " missing entry at index " 
                  << req.prevlogindex() << std::endl;
            return response;
        }
        if(prevEntry.value().term != req.prevlogterm()){
            std::cout << "Node " << this->nodeId << " term mismatch at index " 
                  << req.prevlogindex() << ": expected " << req.prevlogterm() 
                  << ", got " << prevEntry.value().term << std::endl;
            return response;
        }
    }
    std::cout << "Node " << this->nodeId << " passed consistency check for prevLogIndex = " << req.prevlogindex() << std::endl;

    // handle conflicting entries + append
    for(int i = 0; i < req.entries().size(); i++){
        // expected position
        std::uint64_t logIndex = req.prevlogindex() + i + 1;

        auto existingEntry = this->log.getEntry(logIndex);
        if(existingEntry.has_value()){
            const auto& protoEntry = req.entries()[i];
            OpType operation = (protoEntry.operation() == "PUT") ? OpType::PUT : OpType::DELETE;
            
            // check conflict, either term or data
            bool termConflict = (existingEntry.value().term != protoEntry.term());
            bool contentConflict = (existingEntry.value().operation != operation ||
                                  existingEntry.value().key != protoEntry.key() ||
                                  existingEntry.value().value != protoEntry.value());
            
            if(termConflict || contentConflict){
                if(termConflict) {
                    std::cout << "Conflict at index " << logIndex << " for Node " << this->nodeId << " ,term mismatch: " 
                                << existingEntry.value().term << " != " << protoEntry.term() << " ,(expected != received) --> truncating log" << std::endl;
                }
                if(contentConflict) {
                    std::cout << "Conflict at index " << logIndex << " for Node " << this->nodeId << " ,content mismatch: " 
                                << existingEntry.value().key << "=" << existingEntry.value().value << " != " << protoEntry.key() << "=" 
                                << protoEntry.value() << " ,(expected != received) --> truncating log" << std::endl;
                }
                
                // truncate from index and start appending from index
                this->log.truncateFromIndex(logIndex);
                Entry entry(protoEntry.term(), protoEntry.index(), operation, protoEntry.key(), protoEntry.value());
                this->log.appendEntry(entry);
            }
            else{
                std::cout << "Correct entry already at index " << logIndex << " for Node " << this->nodeId << " log" << std::endl;
            }
        }
        else {
            // no entry at this position 
            // convert protobuf entry to Entry struct
            const auto& protoEntry = req.entries()[i];
            OpType operation = (protoEntry.operation() == "PUT") ? OpType::PUT : OpType::DELETE;
            Entry entry(protoEntry.term(), protoEntry.index(), operation, protoEntry.key(), protoEntry.value());
            this->log.appendEntry(entry);
        }
    }

    // update commit index + apply committed entries to state machine
    if(req.leadercommit() > this->commitIndex) {
        std::uint64_t newCommitIndex = std::min(req.leadercommit(), this->log.size());

        for(int i = this->commitIndex + 1; i <= newCommitIndex; i++){
            auto entry = this->log.getEntry(i);
            if(entry.has_value()){
                this->stateMachine.applyEntry(entry.value());
                std::cout << "Applied entry " << i << " to StateMachine of Node " 
                        << this->nodeId << std::endl;
            }
        }
        this->commitIndex = newCommitIndex;
        std::cout << "Updated commitIndex to " << commitIndex 
        << " for Node " << this->nodeId << std::endl;
    }
    response.set_success(true);

    return response;
}

void RaftNode::startNode() {
    {
        std::lock_guard<std::mutex> lock(this->stateMutex);
        if(this->running) return;
        this->running = true;
        this->resetElectionTimer();
    }

    this->electionThread = std::thread(&RaftNode::runElectionTimerLoop, this);
    this->heartbeatThread = std::thread(&RaftNode::runHeartbeatLoop, this);

    std::cout << "Node " << this->nodeId << " started" << std::endl;
}

void RaftNode::stopNode() {
    {
        std::lock_guard<std::mutex> lock(this->stateMutex);
        if(!this->running) return;
        this->running = false;
    }

    if(this->electionThread.joinable()) this->electionThread.join();
    if(this->heartbeatThread.joinable()) this->heartbeatThread.join();

    std::cout << "Node " << this->nodeId << " stopped" << std::endl;
}

void RaftNode::setCluster(RaftCluster* cluster) {
    this->clusterPtr = cluster;
}
