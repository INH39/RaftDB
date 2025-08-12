
#ifndef CLIENT_INTERFACE_H
#define CLIENT_INTERFACE_H

#include "system_interface.h"
#include "client.pb.h"    
#include <memory>
#include <unordered_map>
#include <cstdint>
#include <string>


class ClientInterface {
private:
    SystemInterface* system_;
    std::unordered_map<std::uint64_t, std::unique_ptr<Client::ClientService::Stub>> stubs_;
    std::uint64_t requestId_;

public:
    ClientInterface(SystemInterface* system);
    
    void updateStubs();
    Client::PutResponse put(const std::string& key, const std::string& value, std::uint64_t timeoutMs);
    Client::GetResponse get(const std::string& key, std::uint64_t timeoutMs, Client::ReadConsistency consistency);
    Client::DeleteResponse del(const std::string& key, std::uint64_t timeoutMs);
    Client::ClusterStatusResponse getClusterStatus(std::uint64_t timeoutMs);
    Client::GetStateMachineResponse getStateMachine(std::uint64_t timeoutMs);
    void printStateMachine();
};


#endif // CLIENT_INTERFACE_H