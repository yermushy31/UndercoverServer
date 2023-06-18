
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <thread>
#include <mutex>
#include <winsock2.h>

#include "OpenSSLWrapper.h"
#pragma comment(lib, "ws2_32.lib")

struct ClientPacket {
    SSL* ssl;
    std::string ip;
    std::string name;
    int port;
    bool isconnected;
};

class NewServer {
public:
    NewServer(const std::string& ip, int port);
    ~NewServer();
    void StartServer();
    int Cleanup();

private:
    bool InitWsa();
    void DisplayClients();
    void ReceiveMessage(SSL* ssl);
    void SendAll(int id, const char* data, int totalBytes);
    void HandleInput();

    SOCKET CreateNewSocket(const std::string& ip, int port);

    std::mutex clientMutex;
    std::vector<ClientPacket> connections;
    std::vector<std::thread> receiveThreads;


    bool terminateProgram;
    char recvBuffer[4096];
    char sendBuffer[4096];
    std::string inputStr;

    WSADATA wsadata;
    WORD dllVersion = MAKEWORD(2, 1);
    SOCKET serverSocket;
    std::thread recvThread;
    std::thread inputThread;
    OpenSSLHelper opensslHelper;
    SSL_CTX* sslContext;
};
