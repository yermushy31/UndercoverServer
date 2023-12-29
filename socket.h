
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
#include <fstream>

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
    void Cleanup();

private:
    bool InitWsa();
    void DisplayClients();
    void ReceiveMessage(SSL* ssl);
    void SendMessage(int id, const char* data, int totalBytes);
    void HandleInput();
    void CmdMode(int id, std::string buffer);
    void RenameClient(const std::string inputStr);
    
    SOCKET CreateNewSocket(const std::string& ip, int port);



    int RandomPorts();
    std::string GetNameFromList(int index);
    std::mutex clientMutex;
    std::vector<ClientPacket> connections;
    std::vector<std::thread> receiveThreads;


    int bytes_read;
    int bytes_write;

    const char Path[254] = "audio.wav";

    bool isRecording = false;
    bool cmd_mode = false;
    bool send_file = false;
    bool continueReceiving = true;
    bool terminateProgram;

    char recvFileBuffer[4096];
    char sendFileBuffer[4096];
    char recvBuffer[4096];
    char sendBuffer[4096];

    std::string inputStr;

    SOCKADDR_IN serverAddress;
    SOCKADDR_IN FileTransferAddress;

    WSADATA wsadata;
    WORD dllVersion = MAKEWORD(2, 2);
    SOCKET clientSocket;
    SOCKET serverSocket;
    SOCKET FileSocket;
    std::thread recvThread;
    std::thread inputThread;
    OpenSSLHelper opensslHelper;
    SSL_CTX* fileTransferSslContext;
    SSL* fileTransferSslSocket;
    SSL_CTX* sslContext;


    bool receiveFileData(SSL* ssl, const std::string &filePath);
};
