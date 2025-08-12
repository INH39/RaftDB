
#ifndef NETWORK_LAYER_H
#define NETWORK_LAYER_H

#include <memory>
#include <string>
#include <thread>
#include <grpcpp/grpcpp.h>
#include "client_handler.h"
#include "raft.pb.h"
#include "raft.grpc.pb.h"
#include "client.pb.h"    
#include "client.grpc.pb.h"


// forward declaration
class RaftNode;

// grpc raft service implementation
class RaftServiceImpl final : public Raft::RaftService::Service {
private:
    RaftNode* node_;

public:
    RaftServiceImpl(RaftNode* node) : node_(node) {}
    
    grpc::Status RequestVote(grpc::ServerContext* context, const Raft::RequestVoteRequest* request, Raft::RequestVoteResponse* response) override;                    
    grpc::Status AppendEntries(grpc::ServerContext* context, const Raft::AppendEntriesRequest* request, Raft::AppendEntriesResponse* response) override;
};

// grpc client service implementation
class ClientServiceImpl final : public Client::ClientService::Service {
private:
    std::unique_ptr<ClientHandler> clientHandler_;

public:
    ClientServiceImpl(RaftNode* node);

    grpc::Status Put(grpc::ServerContext* context, const Client::PutRequest* request, Client::PutResponse* response) override;
    grpc::Status Get(grpc::ServerContext* context, const Client::GetRequest* request, Client::GetResponse* response) override;
    grpc::Status Delete(grpc::ServerContext* context, const Client::DeleteRequest* request, Client::DeleteResponse* response) override;
    grpc::Status GetClusterStatus(grpc::ServerContext* context, const Client::ClusterStatusRequest* request, Client::ClusterStatusResponse* response) override;
    grpc::Status GetStateMachine(grpc::ServerContext* context, const Client::GetStateMachineRequest* request, Client::GetStateMachineResponse* response) override;
};

// network layer manager
class NetworkLayer {
private:
    std::unique_ptr<grpc::Server> server_;
    std::unique_ptr<RaftServiceImpl> raftService_;
    std::unique_ptr<ClientServiceImpl> clientService_;
    std::unique_ptr<std::thread> server_thread_;

public:
    void startServer(RaftNode* node, const std::string& address);
    void stopServer();
    
    // send rpcs to other nodes
    Raft::RequestVoteResponse sendRequestVote(const std::string& target_address, const Raft::RequestVoteRequest& request);
    Raft::AppendEntriesResponse sendAppendEntries(const std::string& target_address, const Raft::AppendEntriesRequest& request);
};


#endif // NETWORK_LAYER_H