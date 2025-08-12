
# versions
cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(RaftKV)

include_directories(include)

# pkgconfig to find grpc and protobuf
find_package(PkgConfig REQUIRED)
pkg_check_modules(GRPC REQUIRED grpc++)
pkg_check_modules(PROTOBUF REQUIRED protobuf)

include_directories(${GRPC_INCLUDE_DIRS})
include_directories(${PROTOBUF_INCLUDE_DIRS})

# lib dirs  
link_directories(${GRPC_LIBRARY_DIRS})
link_directories(${PROTOBUF_LIBRARY_DIRS})

# generated protobuf files
set(PROTO_SRCS
    generated/raft.pb.cc
    generated/raft.grpc.pb.cc
    generated/client.pb.cc
    generated/client.grpc.pb.cc
)

# raft implementation src files
set(RAFT_SRCS 
    src/main.cpp 
    src/log.cpp 
    src/entry.cpp 
    src/state_machine.cpp 
    src/raft_node.cpp 
    src/raft_cluster.cpp
    src/network_layer.cpp
    src/client_handler.cpp
    src/system_interface.cpp
    src/client_interface.cpp
)

# target
add_executable(raft_demo 
    ${RAFT_SRCS}
    ${PROTO_SRCS}
)

# link grpc libs
target_include_directories(raft_demo PRIVATE generated)
target_link_libraries(raft_demo ${GRPC_LIBRARIES} ${PROTOBUF_LIBRARIES})
