
#include "network_layer.h"
#include "raft_node.h"
#include "client_handler.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include "raft.pb.h"
#include "client.pb.h"    


// grpc service implementation
grpc::Status RaftServiceImpl::RequestVote(grpc::ServerContext* context, const Raft::RequestVoteRequest* request, Raft::RequestVoteResponse* response) {
    *response = this->node_->handleRequestVote(*request);
    return grpc::Status::OK;
}

grpc::Status RaftServiceImpl::AppendEntries(grpc::ServerContext* context, const Raft::AppendEntriesRequest* request, Raft::AppendEntriesResponse* response) {
    *response = this->node_->handleAppendEntries(*request);
    return grpc::Status::OK;
}

// grpc client service implementation
ClientServiceImpl::ClientServiceImpl(RaftNode* node) {
    this->clientHandler_ = std::make_unique<ClientHandler>(node);
}

grpc::Status ClientServiceImpl::Put(grpc::ServerContext* context, const Client::PutRequest* request, Client::PutResponse* response) {
    *response = this->clientHandler_->handlePut(*request);
    return grpc::Status::OK;
}

grpc::Status ClientServiceImpl::Get(grpc::ServerContext* context, const Client::GetRequest* request, Client::GetResponse* response) {
    *response = this->clientHandler_->handleGet(*request);
    return grpc::Status::OK;
}

grpc::Status ClientServiceImpl::Delete(grpc::ServerContext* context, const Client::DeleteRequest* request, Client::DeleteResponse* response) {
    *response = this->clientHandler_->handleDelete(*request);
    return grpc::Status::OK;
}

grpc::Status ClientServiceImpl::GetClusterStatus(grpc::ServerContext* context, const Client::ClusterStatusRequest* request, Client::ClusterStatusResponse* response) {
    *response = this->clientHandler_->handleClusterStatus(*request);
    return grpc::Status::OK;
}

grpc::Status ClientServiceImpl::GetStateMachine(grpc::ServerContext* context, const Client::GetStateMachineRequest* request, Client::GetStateMachineResponse* response) {
    *response = this->clientHandler_->handleGetStateMachine(*request);
    return grpc::Status::OK;
}

// network layer methods
void NetworkLayer::startServer(RaftNode* node, const std::string& address) {
    this->raftService_ = std::make_unique<RaftServiceImpl>(node);
    this->clientService_ = std::make_unique<ClientServiceImpl>(node);
    
    grpc::ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(this->raftService_.get());
    builder.RegisterService(this->clientService_.get());
    
    this->server_ = builder.BuildAndStart();
    std::cout << "gRPC Server listening on " << address << std::endl;
    
    // start server in background thread
    this->server_thread_ = std::make_unique<std::thread>([this]() {
        this->server_->Wait();
    });
}

void NetworkLayer::stopServer() {
    if (this->server_) {
        this->server_->Shutdown();
    }
    if (this->server_thread_ && this->server_thread_->joinable()) {
        this->server_thread_->join();
    }
}

Raft::RequestVoteResponse NetworkLayer::sendRequestVote(const std::string& target_address, const Raft::RequestVoteRequest& request) {
    auto channel = grpc::CreateChannel(target_address, grpc::InsecureChannelCredentials());
    auto stub = Raft::RaftService::NewStub(channel);
    
    Raft::RequestVoteResponse response;
    grpc::ClientContext context;
    
    grpc::Status status = stub->RequestVote(&context, request, &response);
    
    if (status.ok()) {
        return response;
    } else {
        // return failure response
        Raft::RequestVoteResponse fail_resp;
        fail_resp.set_term(0);
        fail_resp.set_votegranted(false);
        return fail_resp;
    }
}

Raft::AppendEntriesResponse NetworkLayer::sendAppendEntries(const std::string& target_address, const Raft::AppendEntriesRequest& request) {
    auto channel = grpc::CreateChannel(target_address, grpc::InsecureChannelCredentials());
    auto stub = Raft::RaftService::NewStub(channel);
    
    Raft::AppendEntriesResponse response;
    grpc::ClientContext context;
    
    grpc::Status status = stub->AppendEntries(&context, request, &response);
    
    if (status.ok()) {
        return response;
    } else {
        // return failure response
        Raft::AppendEntriesResponse fail_resp;
        fail_resp.set_term(0);
        fail_resp.set_success(false);
        return fail_resp;
    }
}
