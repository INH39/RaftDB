# RaftDB - Distributed Consensus Database

A fault-tolerant distributed key-value database implementing the Raft consensus algorithm to improve my C++ skills and deepen my understanding of distributed systems architecture.

## Features

- **Distributed Consensus**: Implements the Raft algorithm for leader election and log replication
- **Fault Tolerance**: Automatic failover and recovery when nodes go down
- **Key-Value Operations**: Support for PUT, GET, and DELETE operations
- **Multi-Node Architecture**: Distributed across multiple nodes with data consistency
- **gRPC Communication**: Inter-node communication and client interface using Protocol Buffers
- **Persistent Logging**: Durable transaction logs for data recovery

## How to Run

### Built With
- C++17
- CMake 3.16
- Protocol Buffers compiler (`protoc`)
- gRPC C++ libraries

### Build and Run

1. **Generate Protocol Buffer files**
   ```bash
   protoc --cpp_out=generated --grpc_out=generated --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` --proto_path=proto proto/*.proto
   ```

2. **Build the project**
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```

3. **Run the database**
   ```bash
   cd build
   ./raft_demo
   ```

main.cpp file showcases some of the distributed system features
