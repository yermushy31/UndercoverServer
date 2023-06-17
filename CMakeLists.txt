cmake_minimum_required(VERSION 3.25)
project(UndercoverClient)

set(CMAKE_CXX_STANDARD 17)

add_executable(UndercoverClient main.cpp encrypt.cpp encrypt.h)

find_package(OpenSSL REQUIRED)
include_directories(${PROJECT_NAME} PRIVATE ${OPENSSL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENSSL_LIBRARIES})

# Link winsock2
target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
