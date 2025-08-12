
#include "client_interface.h"
#include "client.pb.h"    
#include "client.grpc.pb.h"
#include "system_interface.h"
#include "raft_cluster.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <cstdint>
#include <string>


ClientInterface::ClientInterface(SystemInterface* system) 
                                : system_(system), requestId_(1)
{
    std::unordered_map<std::uint64_t, std::string> nodeAddresses = this->system_->getNodeAddresses();

    for(const auto& [nodeId, address] : nodeAddresses) {
        auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
        this->stubs_[nodeId] = Client::ClientService::NewStub(channel);
    }

    std::cout << "Client Interface initialized with " << this->stubs_.size() << " stubs" << std::endl;
}

// at first, client interface creates stubs for all nodes
// for future node addition, update stubs

void ClientInterface::updateStubs() {
    std::unordered_map<std::uint64_t, std::unique_ptr<Client::ClientService::Stub>> newStubs;
    std::unordered_map<std::uint64_t, std::string> activeAddresses = this->system_->getActiveNodeAddresses();
    
    for(const auto& [nodeId, address] : activeAddresses) {
        auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
        newStubs[nodeId] = Client::ClientService::NewStub(channel);
    }
    this->stubs_ = std::move(newStubs);
    
    std::cout << "Client Interface stubs updated, " << this->stubs_.size() << " active nodes" << std::endl;
}

Client::PutResponse ClientInterface::put(const std::string& key, const std::string& value, std::uint64_t timeoutMs) {

    Client::PutRequest request;
    request.set_key(key);
    request.set_value(value);
    request.set_timeoutms(timeoutMs);
    request.set_requestid(this->requestId_);
    this->requestId_++;

    Client::PutResponse response;
    grpc::ClientContext context;

    for(const auto& [nodeId, stub] : this->stubs_) {
        grpc::Status status = stub->Put(&context, request, &response);

        if(response.has_notleader()) {
            std::cout << "Redirecting node " << nodeId << " to leader (node) " << response.notleader().leaderid() << std::endl;
            grpc::ClientContext redirectContext; // new context for redirect
            
            // check if leader stub exists, node might be stopped
            auto leaderStub = this->stubs_.find(response.notleader().leaderid());
            if(leaderStub != this->stubs_.end()) {
                status = leaderStub->second->Put(&redirectContext, request, &response);
                
                // if redirect to leader fails, client stub may have wrong leader id
                if(!status.ok()) {
                    std::cout << "Redirect to leader failed, trying other nodes..." << std::endl;
                    continue;
                }
            }
        }

        if(status.ok()) {
            return response;
        }
        else {
            std::cout << "PUT request failed on node " << nodeId << ": " << status.error_message() << std::endl;
        }
    }
    return response;
}

Client::GetResponse ClientInterface::get(const std::string& key, std::uint64_t timeoutMs, Client::ReadConsistency consistency) {
    Client::GetRequest request;
    request.set_key(key);
    request.set_timeoutms(timeoutMs);
    request.set_consistency(consistency);
    request.set_requestid(this->requestId_);
    this->requestId_++;

    Client::GetResponse response;
    grpc::ClientContext context;

    for(const auto& [nodeId, stub] : this->stubs_) {
        grpc::Status status = stub->Get(&context, request, &response);

        if(response.has_notleader()) {
            std::cout << "Redirecting node " << nodeId << " to leader (node) " << response.notleader().leaderid() << std::endl;
            grpc::ClientContext redirectContext; // new context for redirect
            
            // check if leader stub exists, node might be stopped
            auto leaderStub = this->stubs_.find(response.notleader().leaderid());
            if(leaderStub != this->stubs_.end()) {
                status = leaderStub->second->Get(&redirectContext, request, &response);
                
                // if redirect to leader fails, client stub may have wrong leader id
                if(!status.ok()) {
                    std::cout << "Redirect to leader failed, trying other nodes..." << std::endl;
                    continue;
                }
            }
        }
        
        if(status.ok()) {
            return response;
        }
        else {
            std::cout << "GET request failed on node " << nodeId << ": " << status.error_message() << std::endl;
        }
    }
    return response;
}

Client::DeleteResponse ClientInterface::del(const std::string& key, std::uint64_t timeoutMs) {
    Client::DeleteRequest request;
    request.set_key(key);
    request.set_timeoutms(timeoutMs);
    request.set_requestid(this->requestId_);
    this->requestId_++;

    Client::DeleteResponse response;
    grpc::ClientContext context;

    for(const auto& [nodeId, stub] : this->stubs_) {
        grpc::Status status = stub->Delete(&context, request, &response);

        if(response.has_notleader()) {
            std::cout << "Redirecting node " << nodeId << " to leader (node) " << response.notleader().leaderid() << std::endl;
            grpc::ClientContext redirectContext; // new context for redirect
            
            // check if leader stub exists, node might be stopped
            auto leaderStub = this->stubs_.find(response.notleader().leaderid());
            if(leaderStub != this->stubs_.end()) {
                status = leaderStub->second->Delete(&redirectContext, request, &response);
                
                // if redirect to leader fails, client stub may have wrong leader id
                if(!status.ok()) {
                    std::cout << "Redirect to leader failed, trying other nodes..." << std::endl;
                    continue;
                }
            }
        }
        
        if(status.ok()) {
            return response;
        }
        else {
            std::cout << "DELETE request failed on node " << nodeId << ": " << status.error_message() << std::endl;
        }
    }
    return response;
}

Client::ClusterStatusResponse ClientInterface::getClusterStatus(std::uint64_t timeoutMs) {
    Client::ClusterStatusRequest request;
    request.set_timeoutms(timeoutMs);
    request.set_requestid(this->requestId_);
    this->requestId_++;

    Client::ClusterStatusResponse response;
    grpc::ClientContext context;

    for(const auto& [nodeId, stub] : this->stubs_) {
        grpc::Status status = stub->GetClusterStatus(&context, request, &response);

        if(status.ok()) {
            return response;
        }
        else {
            std::cout << "GET CLUSTER STATUS request failed on node " << nodeId << ": " << status.error_message() << std::endl;
        }
    }
    return response;
}

Client::GetStateMachineResponse ClientInterface::getStateMachine(std::uint64_t timeoutMs) {
    Client::GetStateMachineRequest request;
    request.set_timeoutms(timeoutMs);
    request.set_requestid(this->requestId_);
    this->requestId_++;

    Client::GetStateMachineResponse response;
    grpc::ClientContext context;

    for(const auto& [nodeId, stub] : this->stubs_) {
        grpc::Status status = stub->GetStateMachine(&context, request, &response);

        if(response.has_notleader()) {
            std::cout << "Redirecting node " << nodeId << " to leader (node) " << response.notleader().leaderid() << std::endl;
            grpc::ClientContext redirectContext; // new context for redirect
            
            // check if leader stub exists, node might be stopped
            auto leaderStub = this->stubs_.find(response.notleader().leaderid());
            if(leaderStub != this->stubs_.end()) {
                status = leaderStub->second->GetStateMachine(&redirectContext, request, &response);
                
                // if redirect to leader fails, client stub may have wrong leader id
                if(!status.ok()) {
                    std::cout << "Redirect to leader failed, trying other nodes..." << std::endl;
                    continue;
                }
            }
        }
        
        if(status.ok()) {
            return response;
        }
        else {
            std::cout << "GET STATE MACHINE request failed on node " << nodeId << ": " << status.error_message() << std::endl;
        }
    }
    return response;
}

void ClientInterface::printStateMachine() {
    Client::GetStateMachineResponse response = this->getStateMachine(5000);
    
    if(response.data_size() == 0) {
        std::cout << "\nState Machine: Empty (no key-value pairs stored)" << std::endl;
        return;
    }
    
    std::cout << "State Machine Contents:" << std::endl;
    std::cout << "--------------------------------------------------------" << std::endl;
    std::cout << "  Term: " << response.currentterm() << std::endl;
    std::cout << "  Commit Index: " << response.commitindex() << std::endl;
    std::cout << "  Total Entries: " << response.data_size() << std::endl;
    std::cout << "--------------------------------------------------------" << std::endl;
    
    for(const auto& kvPair : response.data()) {
        std::cout << "  " << kvPair.key() << " = " << kvPair.value() << std::endl;
    }
    std::cout << "--------------------------------------------------------" << std::endl;
}

