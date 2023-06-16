
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <thread>
#include <mutex>
#include <winsock2.h>

#include "encrypt.h"

#pragma comment(lib, "ws2_32.lib")



class NewServer {
public:
    NewServer(const std::string& ip, int port);
    ~NewServer();
    void StartServer();

private:
    bool InitWsa();
    void DisplayClients();
    void CloseConnection(SOCKET s);
    void ReceiveMessage(SOCKET currentClient);
    void SendAll(int id, const char* data, int totalBytes);
    void HandleInput();

    SOCKET CreateNewSocket(const std::string& ip, int port);

    std::mutex clientMutex;
    std::vector<SOCKET> connections;
    std::string inputStr;

    bool clientMode;

    char recvBuffer[4096];
    char sendBuffer[4096];

    WSADATA wsadata;
    WORD dllVersion = MAKEWORD(2, 1);
    SOCKET serverSocket;
    std::thread inputThread;

};

