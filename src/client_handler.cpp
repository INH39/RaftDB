
#include "client_handler.h"
#include "raft_node.h"
#include "raft_cluster.h"
#include "entry.h"
#include "client.pb.h"
#include <cstdint>
#include <chrono>
#include <thread>
#include <unordered_map> 


Client::NotLeaderInfo ClientHandler::createNotLeaderInfo() {
    Client::NotLeaderInfo info;
    std::unordered_map<std::uint64_t, std::string> nodeAddresses = this->node_->getNodeCluster()->getNodeAddresses();

    for(const auto& [nodeId, address] : nodeAddresses) {
        RaftNode* node = this->node_->getNodeCluster()->getNode(nodeId);

        if(node->getState() == NodeState::LEADER){
            info.set_leaderid(node->getNodeId());
            info.set_leaderaddress(address);
            info.set_term(node->getCurrentTerm());
            break;
        }
    }

    return info;
}

bool ClientHandler::waitForCommit(std::uint64_t entryIndex, std::uint64_t timeout_ms) {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    
    while(this->node_->getCommitIndex() < entryIndex && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return this->node_->getCommitIndex() >= entryIndex;
}

Client::PutResponse ClientHandler::handlePut(const Client::PutRequest& req) {
    Client::PutResponse response;

    if(this->node_->getState() != NodeState::LEADER) {
        response.set_success(false);
        *response.mutable_notleader() = this->createNotLeaderInfo();
        return response;
    }

    std::uint64_t entryIndex = this->node_->getLog()->size() + 1;
    Entry entry(this->node_->getCurrentTerm(), entryIndex, OpType::PUT, req.key(), req.value());

    this->node_->getLog()->appendEntry(entry);
    bool committed = this->waitForCommit(entryIndex, req.timeoutms());

    response.set_success(committed);
    return response;
}

Client::GetResponse ClientHandler::handleGet(const Client::GetRequest& req) {
    Client::GetResponse response;

    // strong -> only leader can serve
    if(req.consistency() == Client::ReadConsistency::STRONG) {
        if(this->node_->getState() != NodeState::LEADER){
            *response.mutable_notleader() = this->createNotLeaderInfo();
            return response;
        }
    }

    // stale -> any node can serve
    auto value = this->node_->getStateMachine()->get(req.key());

    if(value.has_value()) {
        response.set_found(true);
        response.set_value(value.value());
        response.set_term(this->node_->getCurrentTerm());
        response.set_commitindex(this->node_->getCommitIndex());
    }
    else {
        response.set_found(false);
        response.set_term(this->node_->getCurrentTerm());
        response.set_commitindex(this->node_->getCommitIndex());
    }

    return response;
}

Client::DeleteResponse ClientHandler::handleDelete(const Client::DeleteRequest& req) {
    Client::DeleteResponse response;

    if(this->node_->getState() != NodeState::LEADER) {
        response.set_success(false);
        *response.mutable_notleader() = this->createNotLeaderInfo();
        return response;
    }

    auto currentValue = this->node_->getStateMachine()->get(req.key());
    response.set_existed(currentValue.has_value());

    uint64_t entryIndex = this->node_->getLog()->size() + 1;
    Entry entry(this->node_->getCurrentTerm(), entryIndex, OpType::DELETE, req.key(), "");

    this->node_->getLog()->appendEntry(entry);
    bool committed = this->waitForCommit(entryIndex, req.timeoutms());

    response.set_success(committed);
    return response;
}

Client::ClusterStatusResponse ClientHandler::handleClusterStatus(const Client::ClusterStatusRequest& req) {
    Client::ClusterStatusResponse response;
    std::unordered_map<std::uint64_t, std::string> nodeAddresses = this->node_->getNodeCluster()->getNodeAddresses();

    for(const auto& [nodeId, address] : nodeAddresses) {
        RaftNode* node = this->node_->getNodeCluster()->getNode(nodeId);
        Client::NodeInfo* nodeInfo = response.add_nodes();
        
        nodeInfo->set_nodeid(nodeId);
        nodeInfo->set_address(address);
        
        // check if node is running
        if(this->node_->getNodeCluster()->isNodeRunning(nodeId) && node) {
            // node state to string
            if(node->getState() == NodeState::LEADER) nodeInfo->set_role("LEADER");
            else if(node->getState() == NodeState::CANDIDATE) nodeInfo->set_role("CANDIDATE");
            else nodeInfo->set_role("FOLLOWER");
            nodeInfo->set_lastapplied(node->getCommitIndex());

            if(node->getState() == NodeState::LEADER){
                response.set_leaderid(node->getNodeId());
                response.set_leaderaddress(address);
            }
        } 
        else {
            nodeInfo->set_role("STOPPED");
            nodeInfo->set_lastapplied(0);
        }
    }
    response.set_currentterm(this->node_->getCurrentTerm());
    response.set_commitindex(this->node_->getCommitIndex());
    response.set_lastlogindex(this->node_->getLog()->size());

    return response;
}

Client::GetStateMachineResponse ClientHandler::handleGetStateMachine(const Client::GetStateMachineRequest& req) {
    Client::GetStateMachineResponse response;
    
    // strong -> only leader can serve
    if(req.consistency() == Client::ReadConsistency::STRONG) {
        if(this->node_->getState() != NodeState::LEADER){
            *response.mutable_notleader() = this->createNotLeaderInfo();
            return response;
        }
    }
    
    // get state machine data
    const auto& stateMachineData = this->node_->getStateMachine()->getData();
    
    for(const auto& [key, value] : stateMachineData) {
        Client::KeyValuePair* pair = response.add_data();
        pair->set_key(key);
        pair->set_value(value);
    }
    response.set_currentterm(this->node_->getCurrentTerm());
    response.set_commitindex(this->node_->getCommitIndex());
    
    return response;
}
