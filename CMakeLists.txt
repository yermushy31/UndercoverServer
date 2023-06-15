cmake_minimum_required(VERSION 3.25)
project(UndercoverServer)

set(CMAKE_CXX_STANDARD 17)

add_executable(UndercoverServer main.cpp socket.cpp socket.h)
