#include "socket.h"

bool NewServer::InitWsa() {
    if (WSAStartup(dllVersion, &wsadata) != 0) {
        std::cout << "Error WSAstartup " << std::endl;
        return false;
    }
    return true;
}

void NewServer::DisplayClients() {
    if (connections.size() != 0) {
        std::cout << "Which client do you want to connect to?" << std::endl;
        std::lock_guard<std::mutex> lock(clientMutex);
        for (size_t i = 0; i < connections.size(); i++) {
            std::cout << "[" << i << "] Client-" << i << std::endl;
        }
    }
    else {
        std::cout << "No clients are connected at the moment" << std::endl;
    }
}

void NewServer::CloseConnection(SSL* ssl) {
    SSL_shutdown(ssl);
    SSL_free(ssl);
    closesocket(serverSocket);
    WSACleanup();
}

void NewServer::ReceiveMessage(SSL* ssl) {
    std::thread::id currentThreadId = std::this_thread::get_id();
    std::cout << "New Thread Created with the ID: " << currentThreadId << std::endl;
    std::cout << "Client number " << connections.size() - 1 << " got the ID " << currentThreadId << std::endl;
    int res;
    do {
        res = SSL_read(ssl, recvBuffer, sizeof(recvBuffer) - 1);
        if (res == -1)
            std::cout << "RECV error on thread " << currentThreadId << std::endl;
        if (res > 0) {
            recvBuffer[res] = '\0';
            std::cout << "Client number [" << connections.size() - 1 << "] Said: " << recvBuffer << std::endl;
        }
    } while (res > 0);
}

void NewServer::SendAll(int id, const char* data, int totalBytes) {
    std::lock_guard<std::mutex> lock(clientMutex);
    int bytesSent = 0;
    while (bytesSent < totalBytes) {
        int bytesRemaining = totalBytes - bytesSent;
        int bytesToSend = (bytesRemaining < 4096) ? bytesRemaining : 4096;
        if (SSL_write(connections[id], data + bytesSent, bytesToSend) == -1) {
            std::cout << "Failed to send data to client " << id << std::endl;
            break;
        }
        bytesSent += bytesToSend;
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
                        clientMode = true;
                        while (clientMode) {
                            memset(sendBuffer, 0x0, sizeof(sendBuffer));
                            std::cout << "Client~" << id << "#";
                            std::getline(std::cin, inputStr);

                            if (inputStr.find("exit") != std::string::npos) {
                                id = 0;
                                clientMode = false;
                            }
                            SendAll(id, inputStr.c_str(), inputStr.size());
                        }
                    }
                    else {
                        std::cout << "Invalid client ID." << std::endl;
                    }
                }
                catch (const std::exception& e) {
                    std::cout << "Invalid client ID format. Please enter a valid integer." << std::endl;
                }
            }
            else if (inputStr == "clear") {
                system("cls");
            }
        }
    }
}

SOCKET NewServer::CreateNewSocket(const std::string& ip, int port) {
    if (!opensslHelper.Initialize()) {
        std::cout << "Failed to initialize OpenSSL." << std::endl;
        return INVALID_SOCKET;
    }

    if (this->InitWsa()) {
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
    return INVALID_SOCKET;
}

void NewServer::StartServer() {
    sslContext = opensslHelper.CreateContext();
    if (!sslContext) {
        std::cout << "Failed to create SSL context." << std::endl;
        return;
    }
    opensslHelper.ConfigureContext(sslContext);

    inputThread = std::thread(&NewServer::HandleInput, this);
    inputThread.detach();

    while (true) {
        SOCKADDR_IN newClientAddr;
        int newClientLength = sizeof(newClientAddr);
        SOCKET clientSocket = accept(serverSocket, (SOCKADDR*)&newClientAddr, &newClientLength);
        if (clientSocket == INVALID_SOCKET) {
            std::cout << "Accept Error." << std::endl;
            CloseConnection(nullptr);
            return;
        }

        SSL* ssl = SSL_new(sslContext);
        SSL_set_fd(ssl, clientSocket);
        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
            closesocket(clientSocket);
            continue;
        }

        std::lock_guard<std::mutex> lock(clientMutex);
        connections.push_back(ssl);

        std::thread recvThread(&NewServer::ReceiveMessage, this, ssl);
        recvThread.detach();

        std::cout << "New Host connected, IP " << inet_ntoa(newClientAddr.sin_addr) << ", port " << ntohs(newClientAddr.sin_port) << std::endl;
    }
}

NewServer::~NewServer() {
    opensslHelper.Cleanup();
    closesocket(serverSocket);
    WSACleanup();
}

NewServer::NewServer(const std::string& ip, int port) : sslContext(nullptr) {
    serverSocket = CreateNewSocket(ip, port);
}