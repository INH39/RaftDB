
#ifndef RAFT_NODE_H
#define RAFT_NODE_H

#include <vector>
#include <cstdint>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <random>
#include <chrono>
#include <memory>
#include "log.h"
#include "state_machine.h"


// forward declarations
class RaftCluster;
namespace Raft {
    class RequestVoteRequest;
    class RequestVoteResponse; 
    class AppendEntriesRequest;
    class AppendEntriesResponse;
}

enum class NodeState {
    FOLLOWER,
    CANDIDATE,
    LEADER
};

class RaftNode {
private:
    // persistent state (must survive crash)
    std::uint64_t currentTerm;
    std::uint64_t votedFor; // 0 = none
    Log log;

    // volatile state
    NodeState state;
    StateMachine stateMachine;
    std::uint64_t nodeId; // unique
    std::uint64_t commitIndex;
    
    // election (volatile)
    RaftCluster* clusterPtr; // non owning
    std::mutex stateMutex;
    std::atomic<bool> running{false};
    std::thread electionThread;
    std::thread heartbeatThread;

    // timing (volatile)
    std::chrono::steady_clock::time_point lastHeartbeat;
    std::uint64_t heartbeatInterval;
    std::uint64_t electionTimer;
    std::chrono::milliseconds electionTimeoutMin;
    std::chrono::milliseconds electionTimeoutMax;
    std::mt19937 randomGenerator;

    // leader state management (volatile)
    std::vector<std::uint64_t> nextIndex;
    std::vector<std::uint64_t> matchIndex;

    // election methods
    void runElectionTimerLoop();
    void runHeartbeatLoop();
    void resetElectionTimer();
    std::chrono::milliseconds getRandomElectionTimeout();
    void startElection();
    void sendHeartbeats();
    void initializeLeaderState();
    void advanceCommitIndex();

public:
    RaftNode(std::uint64_t nodeId, const std::string& logPath);
    ~RaftNode();

    void becomeFollower(std::uint64_t term);
    void becomeCandidate();
    void becomeLeader();

    NodeState getState() const;
    std::uint64_t getCurrentTerm() const;
    std::uint64_t getNodeId() const;
    std::uint64_t getCommitIndex() const;
    RaftCluster* getNodeCluster();
    StateMachine* getStateMachine();
    Log* getLog();

    Raft::RequestVoteResponse handleRequestVote(const Raft::RequestVoteRequest& req);
    Raft::AppendEntriesResponse handleAppendEntries(const Raft::AppendEntriesRequest& req);

    void startNode();
    void stopNode();
    void setCluster(RaftCluster* cluster);
};


#endif // RAFT_NODE_H
