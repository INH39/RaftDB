
#include "raft_cluster.h"
#include "raft_node.h"
#include "entry.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <cstddef>
#include <unordered_map>
#include "raft.pb.h"


RaftCluster::~RaftCluster() {
    this->stopAllServers();
}

void RaftCluster::addNode(std::uint64_t nodeId, const std::string& logPath, const std::string& address){
    this->nodes.push_back(std::make_unique<RaftNode>(nodeId, logPath));
    this->nodeAddresses[nodeId] = address;
    this->networkLayers[nodeId] = std::make_unique<NetworkLayer>();
    this->getNode(nodeId)->setCluster(this);
}

void RaftCluster::startNodeServer(std::uint64_t nodeId) {
    for (auto& [otherNodeId, networkLayer] : this->networkLayers) {
        if(otherNodeId == nodeId){
            RaftNode* node = this->getNode(nodeId);
            const std::string& address = this->nodeAddresses[nodeId];
            networkLayer->startServer(node, address);
            node->startNode();
            this->nodeStates[nodeId] = true;
            break;
        }
    }
}

void RaftCluster::stopNode(std::uint64_t nodeId) {
    for (auto& [otherNodeId, networkLayer] : this->networkLayers) {
        if(otherNodeId == nodeId){
            RaftNode* node = this->getNode(nodeId);
            node->stopNode();
            networkLayer->stopServer();
            this->nodeStates[nodeId] = false;
            break;
        }
    }
}

void RaftCluster::startAllServers() {
    for (auto& [nodeId, networkLayer] : this->networkLayers) {
        this->startNodeServer(nodeId);
    }
}

void RaftCluster::stopAllServers() {
    for (auto& [nodeId, networkLayer] : this->networkLayers) {
        this->stopNode(nodeId);
    }
}

RaftNode* RaftCluster::getNode(std::uint64_t nodeId) {
    for(auto& node : this->nodes){
        if(node->getNodeId() == nodeId){
            return node.get();
        }
    }
    return nullptr;
}

std::unordered_map<uint64_t, std::string> RaftCluster::getNodeAddresses() const {
    return this->nodeAddresses;
}

bool RaftCluster::isNodeRunning(std::uint64_t nodeId) const {
    auto it = this->nodeStates.find(nodeId);
    return (it != this->nodeStates.end()) && it->second;
}

void RaftCluster::setNodeState(std::uint64_t nodeId, bool isRunning) {
    this->nodeStates[nodeId] = isRunning;
}

Raft::RequestVoteResponse RaftCluster::sendRequestVote(std::uint64_t from, std::uint64_t to, Raft::RequestVoteRequest req){
    auto it = this->nodeAddresses.find(to);
    if (it == this->nodeAddresses.end()) {
        Raft::RequestVoteResponse response;
        response.set_term(0);
        response.set_votegranted(false);
        return response;
    }
    
    const std::string& target_address = it->second;
    auto fromNetworkLayer = this->networkLayers.find(from);
    if (fromNetworkLayer == this->networkLayers.end()) {
        Raft::RequestVoteResponse response;
        response.set_term(0);
        response.set_votegranted(false);
        return response;
    }
    
    return fromNetworkLayer->second->sendRequestVote(target_address, req);
}

Raft::AppendEntriesResponse RaftCluster::sendAppendEntries(std::uint64_t from, std::uint64_t to, Raft::AppendEntriesRequest req){
    auto it = this->nodeAddresses.find(to);
    if (it == this->nodeAddresses.end()) {
        Raft::AppendEntriesResponse response;
        response.set_term(0);
        response.set_success(false);
        return response;
    }
    
    const std::string& target_address = it->second;
    auto fromNetworkLayer = this->networkLayers.find(from);
    if (fromNetworkLayer == this->networkLayers.end()) {
        Raft::AppendEntriesResponse response;
        response.set_term(0);
        response.set_success(false);
        return response;
    }
    
    return fromNetworkLayer->second->sendAppendEntries(target_address, req);
}
