
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <thread>
#include <mutex>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>

#include "OpenSSLWrapper.h"
#pragma comment(lib, "ws2_32.lib")

struct ClientPacket {
    SSL* ssl;
    std::string ip;
    std::string name;
    int port;
    bool isconnected;
};


struct ServerPacket {

};

class NewServer {
public:
    NewServer(const std::string& ip, int port);
    ~NewServer();
    void StartServer();
    void Cleanup();

private:
    bool InitWsa();
    void DisplayClients();
    void ReceiveMessage(SSL* ssl);
    void SendMessage(int id, const char* data, int totalBytes);
    void HandleInput();

    SOCKET CreateNewSocket(const std::string& ip, int port);
    SOCKET clientSocket;
    std::mutex clientMutex;
    std::vector<ClientPacket> connections;
    std::vector<std::thread> receiveThreads;

    bool terminateProgram;
    char recvBuffer[4096];
    char sendBuffer[4096];
    std::string inputStr;
    WSADATA wsadata;
    WORD dllVersion = MAKEWORD(2, 2);
    SOCKET serverSocket;
    std::thread recvThread;
    std::thread inputThread;
    OpenSSLHelper opensslHelper;
    SSL_CTX* sslContext;
};
