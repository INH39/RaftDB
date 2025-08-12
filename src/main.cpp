#include "system_interface.h"
#include "client.pb.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <string>

// ---------------------------------------------------------------------------
// RAFT DISTRIBUTED KEY-VALUE DATABASE SHOWCASE 
// ---------------------------------------------------------------------------
// Implementation of the Raft consensus algorithm
// providing a fault-tolerant, distributed key-value store with:
//   - Data consistency 
//   - Automatic leader election and failover
//   - Log replication across multiple nodes
//   - Client operations: PUT, GET, DELETE
//   - Dynamic cluster membership
// ---------------------------------------------------------------------------


//  ------------------------------- Helpers -------------------------------

void printBanner() {
    std::cout << "\n" << std::string(80, '-') << std::endl;
    std::cout << "RAFT DISTRIBUTED KEY-VALUE DATABASE SHOWCASE" << std::endl;
    std::cout << std::string(80, '-') << std::endl;
    std::cout << "Building a fault-tolerant mini distributed database ..." << std::endl;
    std::cout << std::string(80, '-') << std::endl;
}

void printSection(const std::string& title) {
    std::cout << "\n" << std::string(60, '-') << std::endl;
    std::cout << title << std::endl;
    std::cout << std::string(60, '-') << std::endl;
}

void waitForStabilization(int seconds = 2) {
    std::cout << "Waiting " << seconds << "s for consensus ..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
}

// -------------------------------------------------------------------------


// Simple showcase demonstrating distributed database functionality
void runDistributedDatabaseShowcase() {
    printBanner();
    
    printSection("1. CLUSTER INITIALIZATION");
    std::cout << "Creating a 3-node Raft cluster ..." << std::endl;
    
    // Initialize distributed system with 3 initial nodes
    // This sytem can take maximum 5 nodes
    SystemInterface system(3, 5, 50050, "localhost", "../logs");
    
    std::cout << "Nodes created:" << std::endl;
    std::cout << "   - Node 1 at localhost:50051" << std::endl;
    std::cout << "   - Node 2 at localhost:50052" << std::endl;
    std::cout << "   - Node 3 at localhost:50053" << std::endl;
    
    waitForStabilization(3);
    std::cout << "Cluster ready with automatic leader election ..." << std::endl;
    
    // Show cluster status
    auto status = system.getClusterStatus(5000);
    if (status.leaderid() > 0) {
        std::cout << "Leader: Node " << status.leaderid() << " at " << status.leaderaddress() << std::endl;
        std::cout << "Total Nodes: " << status.nodes_size() << std::endl;
    }
    
    printSection("2. DISTRIBUTED DATA OPERATIONS");
    std::cout << "Demonstrating distributed key-value operations ..." << std::endl;
    
    // Store data across the distributed cluster
    std::cout << "\nStoring data (replicated across all nodes):" << std::endl;
    auto put1 = system.put("user:1", "Alice Johnson", 5000);
    std::cout << "   PUT user:1 = 'Alice Johnson': " << (put1.success() ? "Success" : "Failed") << std::endl;
    
    auto put2 = system.put("user:2", "Bob Smith", 5000);
    std::cout << "   PUT user:2 = 'Bob Smith': " << (put2.success() ? "Success" : "Failed") << std::endl;
    
    auto put3 = system.put("config:timeout", "30000", 5000);
    std::cout << "   PUT config:timeout = '30000': " << (put3.success() ? "Success" : "Failed") << std::endl;
    
    waitForStabilization(1);
    
    // Retrieve data, check consistency
    std::cout << "\nReading data (check consistency across cluster):" << std::endl;
    auto get1 = system.get("user:1", 5000, Client::ReadConsistency::STRONG);
    std::cout << "   GET user:1 -> " << (get1.found() ? "Success, value found: " + get1.value() : "Failed: Not found") << std::endl;
    
    auto get2 = system.get("user:2", 5000, Client::ReadConsistency::STRONG);
    std::cout << "   GET user:2 -> " << (get2.found() ? "Success, value found: " + get2.value() : "Failed: Not found") << std::endl;
    
    auto get3 = system.get("config:timeout", 5000, Client::ReadConsistency::STRONG);
    std::cout << "   GET config:timeout -> " << (get3.found() ? "Success, value found: " + get3.value() : "Failed: Not found") << std::endl;
    
    printSection("3. FAULT TOLERANCE DEMONSTRATION");
    std::cout << "Testing distributed system fault tolerance ..." << std::endl;
    
    // Update existing data
    std::cout << "\nUpdating user data:" << std::endl;
    auto update = system.put("user:1", "Alice Johnson-Updated", 5000);
    std::cout << "   UPDATE user:1: " << (update.success() ? "Success" : "Failed") << std::endl;
    
    waitForStabilization(1);
    
    // Verify update was replicated
    auto verify = system.get("user:1", 5000, Client::ReadConsistency::STRONG);
    std::cout << "   VERIFY user:1 -> " << (verify.found() ? "Success, value found: " + verify.value() : "Failed: Not found") << std::endl;
    
    // Delete operation
    std::cout << "\nDeleting data:" << std::endl;
    auto del = system.del("config:timeout", 5000);
    std::cout << "   DELETE config:timeout: " << (del.success() ? "Success" : "Failed") << std::endl;
    
    waitForStabilization(1);
    
    // Verify deletion
    auto verifyDel = system.get("config:timeout", 5000, Client::ReadConsistency::STRONG);
    std::cout << "   VERIFY deletion: " << (!verifyDel.found() ? "Successfully deleted" : "Failed: Still exists") << std::endl;
    
    printSection("4. FINAL CLUSTER STATE");
    std::cout << "Distributed database final state:" << std::endl;
    
    // Show final cluster status
    auto finalStatus = system.getClusterStatus(5000);
    if (finalStatus.leaderid() > 0) {
        std::cout << "   Term: " << finalStatus.currentterm() << std::endl;
        std::cout << "   Leader: Node " << finalStatus.leaderid() << std::endl;
        std::cout << "   Commit Index: " << finalStatus.commitindex() << std::endl;
        std::cout << "   Total Nodes: " << finalStatus.nodes_size() << std::endl;
    }
    
    // Show final state machine contents
    auto stateMachine = system.getStateMachine(5000);
    if (stateMachine.data_size() > 0) {
        std::cout << "\nFinal Database Contents:" << std::endl;
        std::cout << "   Total Records: " << stateMachine.data_size() << std::endl;
        for (const auto& entry : stateMachine.data()) {
            std::cout << "   " << entry.key() << " = " << entry.value() << std::endl;
        }
    }
    
    std::cout << "\n" << std::string(80, '-') << std::endl;
    std::cout << "SHOWCASE COMPLETE" << std::endl;
    std::cout << "Demonstrated functionalities:" << std::endl;
    std::cout << "   - Distributed consensus (Raft algorithm)" << std::endl;
    std::cout << "   - Fault-tolerant data replication" << std::endl;
    std::cout << "   - Automatic leader election" << std::endl;
    std::cout << "   - Data consistency " << std::endl;
    std::cout << "   - CRUD operations (Create, Read, Update, Delete)" << std::endl;
    std::cout << std::string(80, '-') << std::endl;
    
    std::cout << "\nSystem will shutdown in 5 seconds ..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    system.stopSystem();
    std::cout << "Distributed system shutdown complete" << std::endl;
}

int main() {
    std::cout << "\nRaft Distributed Database Demo" << std::endl;
    std::cout << "This showcase demonstrates a distributed key-value store" << std::endl;
    std::cout << "\nPress Enter to start the demonstration ..." << std::endl;
    std::cin.get();
    
    try {
        runDistributedDatabaseShowcase();
    } catch (const std::exception& e) {
        std::cerr << "Showcase failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\nDemo complete" << std::endl;
    return 0;
}