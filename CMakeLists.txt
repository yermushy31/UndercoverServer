cmake_minimum_required(VERSION 3.25)
project(UndercoverClient)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")

add_executable(UndercoverClient main.cpp)

find_package(OpenSSL REQUIRED)
include_directories(${PROJECT_NAME} PRIVATE ${OPENSSL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENSSL_LIBRARIES})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static -lpthread")
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

# Link winsock2
target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
