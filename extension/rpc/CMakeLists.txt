cmake_minimum_required(VERSION 2.8.12)

project(RpcExtension)

add_extension_definitions()

include_directories(include ../..)

add_library(rpc_extension STATIC rpc-extension.cpp)
