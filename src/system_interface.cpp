
#include "system_interface.h"
#include "client_interface.h"
#include "raft_cluster.h"
#include "client.pb.h"  
#include <cstdint>
#include <string>
#include <unordered_map>
#include <iostream>


SystemInterface::SystemInterface(std::uint64_t initialNumNodes, std::uint64_t maxNumNodes, std::uint64_t startPort, 
                                 const std::string& serverAddress, const std::string& logDirectory)
                                : cluster_(std::make_unique<RaftCluster>()), maxNumNodes_(maxNumNodes), 
                                  startPort_(startPort), usedIds_(), nodeStates_(),
                                  serverAddress_(serverAddress), logDirectory_(logDirectory), printSystemTimeout_(5000),
                                  clientInterface_(nullptr)
                                
{
    // init nodes with sequential ids
    for(int i = 1; i <= initialNumNodes; i++){
        this->addNode(i);
    }

    std::cout << "System Interface initialized" << std::endl;

    // at first, client interface creates stubs for all nodes
    this->clientInterface_ = new ClientInterface(this);
} 

SystemInterface::~SystemInterface() {
    delete this->clientInterface_;
    this->stopSystem();
}

void SystemInterface::addNode(std::uint64_t nodeId) {
    std::string logFile = this->logDirectory_;
    if(!(!logFile.empty() && logFile.back() == '/')) logFile += '/';
    logFile += "client_node" + std::to_string(nodeId);

    std::string nodeAddress = this->serverAddress_ + ":" + std::to_string(this->startPort_ + nodeId);

    this->cluster_->addNode(nodeId, logFile, nodeAddress);
    this->usedIds_[nodeId] = true; 
    this->nodeStates_[nodeId] = true; 
    this->cluster_->startNodeServer(nodeId);

    // for future node addition, update stubs
    if (this->clientInterface_) {
        this->clientInterface_->updateStubs();
    }
}

void SystemInterface::stopNode(std::uint64_t nodeId) {
    this->cluster_->stopNode(nodeId);
    this->usedIds_[nodeId] = false;
    this->nodeStates_[nodeId] = false;

    if (this->clientInterface_) {
        this->clientInterface_->updateStubs();
    }
}

void SystemInterface::printSystemInformation() {
    Client::ClusterStatusResponse statusResp = this->clientInterface_->getClusterStatus(this->printSystemTimeout_);

    // valid cluster status has non-zero term and a leader
    if(statusResp.currentterm() == 0 || statusResp.leaderid() == 0) {
        std::cout << "Invalid cluster status, could not print cluster status" << std::endl;
        return;
    }

    std::cout << "Cluster Status:" << std::endl;
    std::cout << "  Term: " << statusResp.currentterm() << std::endl;
    std::cout << "  Leader: Node " << statusResp.leaderid() << " at " << statusResp.leaderaddress() << std::endl;
    std::cout << "  Commit Index: " << statusResp.commitindex() << std::endl;
    std::cout << "  Log Size: " << statusResp.lastlogindex() << std::endl;
    std::cout << "  Nodes:" << std::endl;
    for (const auto& node : statusResp.nodes()) {
        std::cout << "    Node " << node.nodeid() << ": " << node.role() 
                  << " (" << node.address() << ")" << std::endl;
    }
}

void SystemInterface::printStateMachine() {
    this->clientInterface_->printStateMachine();
}

void SystemInterface::stopSystem() {
    this->cluster_->stopAllServers();
}

std::unordered_map<uint64_t, std::string> SystemInterface::getNodeAddresses() const {
    return this->cluster_->getNodeAddresses();
}

std::unordered_map<uint64_t, std::string> SystemInterface::getActiveNodeAddresses() const {
    std::unordered_map<uint64_t, std::string> activeAddresses;
    auto allAddresses = this->cluster_->getNodeAddresses();
    
    for (const auto& [nodeId, address] : allAddresses) {
        if (this->isNodeRunning(nodeId)) {
            activeAddresses[nodeId] = address;
        }
    }

    return activeAddresses;
}

bool SystemInterface::isNodeRunning(std::uint64_t nodeId) const {
    auto it = this->nodeStates_.find(nodeId);
    return (it != this->nodeStates_.end() && it->second);
}


//------------------------------- for testing mostly, a client would use the client interface -------------------------------

Client::PutResponse SystemInterface::put(const std::string& key, const std::string& value, std::uint64_t timeoutMs) {
    return this->clientInterface_->put(key, value, timeoutMs);
}

Client::GetResponse SystemInterface::get(const std::string& key, std::uint64_t timeoutMs, Client::ReadConsistency consistency) {
    return this->clientInterface_->get(key, timeoutMs, consistency);
}

Client::DeleteResponse SystemInterface::del(const std::string& key, std::uint64_t timeoutMs) {
    return this->clientInterface_->del(key, timeoutMs);
}

Client::ClusterStatusResponse SystemInterface::getClusterStatus(std::uint64_t timeoutMs) {
    return this->clientInterface_->getClusterStatus(timeoutMs);
}

Client::GetStateMachineResponse SystemInterface::getStateMachine(std::uint64_t timeoutMs) {
    return this->clientInterface_->getStateMachine(timeoutMs);
}
