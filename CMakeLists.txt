cmake_minimum_required(VERSION 3.25)
project(UndercoverServer)

set(CMAKE_CXX_STANDARD 17)

add_executable(UndercoverServer main.cpp socket.cpp socket.h encrypt.cpp encrypt.h prototype.cpp)

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARIES})