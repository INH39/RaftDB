
#ifndef RAFT_CLUSTER_H
#define RAFT_CLUSTER_H

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include "entry.h"
#include "network_layer.h"
#include "raft.pb.h"


// forward declaration
class RaftNode;

class RaftCluster {
private:
    std::vector<std::unique_ptr<RaftNode>> nodes;
    std::unordered_map<uint64_t, std::string> nodeAddresses;
    std::unordered_map<uint64_t, std::unique_ptr<NetworkLayer>> networkLayers;
    std::unordered_map<uint64_t, bool> nodeStates; // running or stopped

public:
    ~RaftCluster();

    void addNode(std::uint64_t nodeId, const std::string& logPath, const std::string& address);
    void startNodeServer(std::uint64_t nodeId);
    void stopNode(std::uint64_t nodeId);
    void startAllServers();
    void stopAllServers();
    RaftNode* getNode(std::uint64_t nodeId);
    std::unordered_map<uint64_t, std::string> getNodeAddresses() const;
    bool isNodeRunning(std::uint64_t nodeId) const;
    void setNodeState(std::uint64_t nodeId, bool isRunning);
    Raft::RequestVoteResponse sendRequestVote(std::uint64_t from, std::uint64_t to, Raft::RequestVoteRequest req);
    Raft::AppendEntriesResponse sendAppendEntries(std::uint64_t from, std::uint64_t to, Raft::AppendEntriesRequest req);
};


#endif // RAFT_CLUSTER_H