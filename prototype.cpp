//
// Created by yermushy on 16/06/2023.
//
#include <iostream>
#include <sstream>
#include <thread>
#include <mutex>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")

#define DEFAULT_BUFLEN 1024

class NewServer {
public:
    NewServer(const std::string& ip, int port);
    ~NewServer();

    void StartServer();

private:
    bool InitWsa();
    void CloseConnection(SOCKET s);
    void ReceiveMessage(SSL* ssl);
    void SendAll(SSL* ssl, const char* data, int totalBytes);
    void HandleInput();

    SOCKET CreateNewSocket(const std::string& ip, int port);

    WSADATA wsadata;
    WORD dllVersion = MAKEWORD(2, 2);
    SOCKET serverSocket;
    std::vector<SSL*> connections;
    std::mutex clientMutex;
    std::thread inputThread;
    std::string inputStr;
    char recvBuffer[DEFAULT_BUFLEN];
    char sendBuffer[DEFAULT_BUFLEN];
};

bool NewServer::InitWsa() {
    if (WSAStartup(dllVersion, &wsadata) != 0) {
        std::cout << "Error WSAstartup " << std::endl;
        return false;
    }
    return true;
}

void NewServer::CloseConnection(SOCKET s) {
    closesocket(s);
    WSACleanup();
}

void NewServer::ReceiveMessage(SSL* ssl) {
    std::thread::id currentThreadId = std::this_thread::get_id();
    std::cout << "New Thread Created with the ID: " << currentThreadId << std::endl;

    int res;
    do {
        res = SSL_read(ssl, recvBuffer, sizeof(recvBuffer) - 1);
        if (res <= 0) {
            int err = SSL_get_error(ssl, res);
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
                continue;

            std::cout << "RECV error on thread " << currentThreadId << std::endl;
            break;
        }

        recvBuffer[res] = '\0';
        std::cout << "Received: " << recvBuffer << std::endl;
    } while (res > 0);
}

void NewServer::SendAll(SSL* ssl, const char* data, int totalBytes) {
    std::lock_guard<std::mutex> lock(clientMutex);
    int bytesSent = 0;
    while (bytesSent < totalBytes) {
        int bytesRemaining = totalBytes - bytesSent;
        int bytesToSend = (bytesRemaining < DEFAULT_BUFLEN) ? bytesRemaining : DEFAULT_BUFLEN;
        int sent = SSL_write(ssl, data + bytesSent, bytesToSend);
        if (sent <= 0) {
            int err = SSL_get_error(ssl, sent);
            if (err == SSL_ERROR_WANT_WRITE)
                continue;

            std::cout << "Failed to send data" << std::endl;
            break;
        }
        bytesSent += sent;
    }
}

void NewServer::HandleInput() {
    while (inputStr != "quit") {
        if (!connections.empty()) {
            std::cout << "server~# ";
            std::getline(std::cin, inputStr);
        }
        if (inputStr.substr(0, 7) == "connect") {
            std::istringstream iss(inputStr);
            std::string command;
            iss >> command;

            std::string clientId;
            iss >> clientId;
            std::cin.ignore();

            if (!clientId.empty()) {
                try {
                    int id = std::stoi(clientId);
                    if (id >= 0 && id < connections.size()) {
                        while (true) {
                            memset(sendBuffer, 0x0, sizeof(sendBuffer));
                            std::cout << "Client~" << id << "#";
                            std::getline(std::cin, inputStr);

                            if (inputStr.find("exit") != std::string::npos)
                                break;

                            SendAll(connections[id], inputStr.c_str(), inputStr.size());
                        }
                    } else {
                        std::cout << "Invalid client ID." << std::endl;
                    }
                } catch (const std::exception& e) {
                    std::cout << "Invalid client ID format. Please enter a valid integer." << std::endl;
                }
            } else if (inputStr == "clear") {
                system("cls");
            }
        }
    }
}

SOCKET NewServer::CreateNewSocket(const std::string& ip, int port) {
    if (!InitWsa())
        return INVALID_SOCKET;

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cout << "Invalid Socket." << std::endl;
        WSACleanup();
        return INVALID_SOCKET;
    }

    SOCKADDR_IN serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(ip.c_str());
    serverAddress.sin_port = htons(port);

    if (bind(sock, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cout << "Bind Error." << std::endl;
        closesocket(sock);
        WSACleanup();
        return INVALID_SOCKET;
    }

    if (listen(sock, SOMAXCONN) == SOCKET_ERROR) {
        std::cout << "Listen Error." << std::endl;
        closesocket(sock);
        WSACleanup();
        return INVALID_SOCKET;
    }

    std::cout << "Listening..." << std::endl;
    return sock;
}

void NewServer::StartServer() {
    inputThread = std::thread(&NewServer::HandleInput, this);
    inputThread.detach();

    SSL_library_init();
    SSL_CTX* sslContext = SSL_CTX_new(TLS_server_method());
    if (sslContext == nullptr) {
        std::cout << "SSL context creation failed" << std::endl;
        CloseConnection(serverSocket);
        return;
    }

    SSL_CTX_use_certificate_file(sslContext, "server.crt", SSL_FILETYPE_PEM);
    SSL_CTX_use_PrivateKey_file(sslContext, "server.key", SSL_FILETYPE_PEM);

    while (true) {
        SOCKADDR_IN newClientAddr;
        int newClientLength = sizeof(newClientAddr);
        SOCKET clientSocket = accept(serverSocket, (SOCKADDR*)&newClientAddr, &newClientLength);
        if (clientSocket == INVALID_SOCKET) {
            std::cout << "Accept Error." << std::endl;
            CloseConnection(serverSocket);
            SSL_CTX_free(sslContext);
            return;
        }

        SSL* ssl = SSL_new(sslContext);
        if (ssl == nullptr) {
            std::cout << "SSL object creation failed" << std::endl;
            CloseConnection(serverSocket);
            SSL_CTX_free(sslContext);
            return;
        }

        if (SSL_set_fd(ssl, clientSocket) != 1) {
            std::cout << "SSL set file descriptor failed" << std::endl;
            CloseConnection(serverSocket);
            SSL_free(ssl);
            SSL_CTX_free(sslContext);
            return;
        }

        if (SSL_accept(ssl) != 1) {
            std::cout << "SSL handshake failed" << std::endl;
            CloseConnection(serverSocket);
            SSL_free(ssl);
            SSL_CTX_free(sslContext);
            return;
        }

        std::lock_guard<std::mutex> lock(clientMutex);
        connections.push_back(ssl);

        std::thread recvThread(&NewServer::ReceiveMessage, this, ssl);
        recvThread.detach();

        std::cout << "New Host connected, IP " << inet_ntoa(newClientAddr.sin_addr) << ", port " << ntohs(newClientAddr.sin_port) << std::endl;
    }

    SSL_CTX_free(sslContext);
}

NewServer::~NewServer() {
    for (SSL* ssl : connections)
        SSL_free(ssl);
    closesocket(serverSocket);
    WSACleanup();
}

NewServer::NewServer(const std::string& ip, int port) {
    serverSocket = CreateNewSocket(ip, port);
}

int main() {
    std::string ip = "127.0.0.1";
    int port = 8080;

    NewServer server(ip, port);
    server.StartServer();

    return 0;
}
