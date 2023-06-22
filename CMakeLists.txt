cmake_minimum_required(VERSION 3.25)
project(UndercoverServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")

# Add the source files
set(SOURCES main.cpp socket.cpp OpenSSLWrapper.cpp socket.h OpenSSLWrapper.h sessions.h sessions.cpp)

# Create the executable target
add_executable(UndercoverServer ${SOURCES})

# Link OpenSSL
find_package(OpenSSL REQUIRED)
target_include_directories(UndercoverServer PRIVATE ${OPENSSL_INCLUDE_DIR})
target_link_libraries(UndercoverServer PRIVATE ${OPENSSL_LIBRARIES})
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static -lpthread")
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

# Link winsock2
target_link_libraries(UndercoverServer PRIVATE ws2_32 OpenSSL::SSL OpenSSL::Crypto)
