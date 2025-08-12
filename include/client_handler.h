
#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include "raft_node.h"
#include "client.pb.h"
#include <cstdint>


class ClientHandler {
private:
    RaftNode* node_;

    Client::NotLeaderInfo createNotLeaderInfo();
    bool waitForCommit(std::uint64_t entryIndex, std::uint64_t timeout_ms);

public:
    ClientHandler(RaftNode* node) :node_(node) {}

    Client::PutResponse handlePut(const Client::PutRequest& req);
    Client::GetResponse handleGet(const Client::GetRequest& req);
    Client::DeleteResponse handleDelete(const Client::DeleteRequest& req);
    Client::ClusterStatusResponse handleClusterStatus(const Client::ClusterStatusRequest& req);
    Client::GetStateMachineResponse handleGetStateMachine(const Client::GetStateMachineRequest& req);
};


#endif // CLIENT_HANDLER_H