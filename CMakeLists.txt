cmake_minimum_required(VERSION 3.25)
project(UndercoverServer)

set(CMAKE_CXX_STANDARD 17)

# Add the source files
set(SOURCES main.cpp socket.cpp OpenSSLWrapper.cpp socket.h OpenSSLWrapper.h)

# Create the executable target
add_executable(UndercoverServer ${SOURCES})

# Link OpenSSL
find_package(OpenSSL REQUIRED)
target_include_directories(UndercoverServer PRIVATE ${OPENSSL_INCLUDE_DIR})
target_link_libraries(UndercoverServer PRIVATE ${OPENSSL_LIBRARIES})

# Link winsock2
target_link_libraries(UndercoverServer PRIVATE ws2_32 OpenSSL::SSL OpenSSL::Crypto)
