
#ifndef SYSTEM_INTERFACE_H
#define SYSTEM_INTERFACE_H

#include "raft_cluster.h"
#include <cstdint>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

// forward declaration
class ClientInterface;

class SystemInterface {
private:
    std::unique_ptr<RaftCluster> cluster_;
    std::uint64_t maxNumNodes_;
    std::uint64_t startPort_; // node port -> start port + node id
    std::unordered_map<std::uint64_t, bool> usedIds_;
    std::unordered_map<std::uint64_t, bool> nodeStates_; // running or stopped
    std::string serverAddress_;
    std::string logDirectory_;
    std::uint64_t printSystemTimeout_; // default timeout for printSystemInformation()
    ClientInterface* clientInterface_;

public:
    SystemInterface(std::uint64_t initialNumNodes, std::uint64_t maxNumNodes, std::uint64_t startPort, 
                    const std::string& serverAddress, const std::string& logDirectory);
    ~SystemInterface();

    void addNode(std::uint64_t nodeId);
    void stopNode(std::uint64_t nodeId);
    void printSystemInformation();
    void printStateMachine();
    void stopSystem();
    std::unordered_map<uint64_t, std::string> getNodeAddresses() const;
    std::unordered_map<uint64_t, std::string> getActiveNodeAddresses() const;
    bool isNodeRunning(std::uint64_t nodeId) const;
    
    //------------------------------- for testing mostly, a client would use the client interface -------------------------------
    Client::PutResponse put(const std::string& key, const std::string& value, std::uint64_t timeoutMs = 5000);
    Client::GetResponse get(const std::string& key, std::uint64_t timeoutMs = 5000, Client::ReadConsistency consistency = Client::ReadConsistency::STRONG);
    Client::DeleteResponse del(const std::string& key, std::uint64_t timeoutMs = 5000);
    Client::ClusterStatusResponse getClusterStatus(std::uint64_t timeoutMs = 5000);
    Client::GetStateMachineResponse getStateMachine(std::uint64_t timeoutMs = 5000);
};


#endif // SYSTEM_INTERFACE_H