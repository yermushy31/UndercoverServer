#include "socket.h"

NewServer::NewServer(const std::string &ip, int port) : sslContext(nullptr), terminateProgram(false) {
    serverSocket = CreateNewSocket(ip, port);
}

NewServer::~NewServer() {
    Cleanup();
}

bool NewServer::InitWsa() {
    if (WSAStartup(dllVersion, &wsadata) != 0) {
        std::cout << "Error WSAstartup " << std::endl;
        return false;
    }
    return true;
}

std::string NewServer::GetNameFromList(int index) {
    return connections[index].name;
}


void NewServer::RenameClient(const std::string inputStr) {
    std::istringstream iss(inputStr);
    std::string command;
    iss >> command;
    std::string clientId;
    iss >> clientId;
    std::string newname;
    iss >> newname;

    if (!clientId.empty() && !newname.empty()) {
        try {
            int id = std::stoi(clientId);
            if (id >= 0 && id < connections.size()) {
                connections[id].name = newname;
                std::cout << "Client " << id << " name updated to: " << connections[id].name << std::endl;
            } else {
                std::cout << "Invalid client ID." << std::endl;
            }
        } catch (const std::exception &e) {
            std::cout << "Invalid client ID format. Please enter a valid integer." << std::endl;
        }
    } else {
        std::cout << "Please provide both client ID and new name." << std::endl;
    }
}

void NewServer::DisplayClients() {
    if (connections.size() != 0) {
        std::cout << "Clients List >>>" << std::endl;
        std::lock_guard<std::mutex> lock(clientMutex);
        for (size_t i = 0; i < connections.size(); i++) {
            std::cout << "[" << i << "] " << connections[i].name << " " << connections[i].ip << ":" << connections[i].port << std::endl;
        }
    } else {
        std::cout << "No clients are connected at the moment" << std::endl;
    }
}


void NewServer::CmdMode(int id, std::string buffer) {
    std::cout << "Client~" << id << "[CMD-mode]>#";
    std::getline(std::cin, buffer);
    if (buffer.find("exit") != std::string::npos) {
        cmd_mode = false;
    }
    SendMessage(id, buffer.c_str(), buffer.size());
}

void NewServer::ReceiveMessage(SSL *ssl) {
    int res;
    bool continueReceiving = true;

    while (continueReceiving) {
        res = SSL_read(ssl, recvBuffer, sizeof(recvBuffer) - 1);
        if (res > 0) {
            recvBuffer[res] = '\0';
            std::cout.write(recvBuffer, res);
        } else {
            continueReceiving = false;
        }
    }
    connections[connections.size() - 1].isconnected = false;
    std::cout << "Client number [" << connections.size() - 1 << "] disconnected" << std::endl;
    std::lock_guard<std::mutex> lock(clientMutex);
    connections.erase(connections.begin() + connections.size() - 1);
}

void NewServer::SendMessage(int id, const char *data, int totalBytes) {
    std::lock_guard<std::mutex> lock(clientMutex);
    int bytesSent = 0;
    while (bytesSent < totalBytes) {
        int bytesRemaining = totalBytes - bytesSent;
        int bytesToSend = (bytesRemaining < 4096) ? bytesRemaining : 4096;
        if (SSL_write(connections[id].ssl, data + bytesSent, bytesToSend) == -1) {
            std::cout << "Failed to send data to client " << id << std::endl;
            break;
        }
        bytesSent += bytesToSend;
    }
}

void NewServer::HandleInput() {
    std::cout << "Listening..." << std::endl;
    do {
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
            if (!clientId.empty()) {
                try {
                    int id = std::stoi(clientId);
                    if (id >= 0 && id < connections.size()) {
                        while (connections[id].isconnected) {
                            memset(sendBuffer, 0x0, sizeof(sendBuffer));
                            std::cout << "Client~" << id << "#";
                            std::getline(std::cin, inputStr);

                            if (inputStr.find("quit") != std::string::npos) {
                                id = 0;
                                break;
                            }
                            if (inputStr.find("scmd") != std::string::npos) {
                                cmd_mode = true;
                                SendMessage(id, inputStr.c_str(), inputStr.size());
                                while (cmd_mode) {
                                    this->CmdMode(id, inputStr);
                                }
                            }
                            SendMessage(id, inputStr.c_str(), inputStr.size());
                        }
                    } else {
                        std::cout << "Invalid client ID." << std::endl;
                    }
                } catch (const std::exception &e) {
                    std::cout << "Invalid client ID format. Please enter a valid integer." << std::endl;
                }
            }
        } else if (inputStr == "sclient") {
            this->DisplayClients();
        } else if (inputStr.substr(0, 8) == "renamecl") {
            this->RenameClient(inputStr);
        } else if (inputStr == "clear") {
            system("cls");
        } else if (inputStr == "abort") {
            this->Cleanup();
        }
    } while (!terminateProgram);
}

SOCKET NewServer::CreateNewSocket(const std::string &ip, int port) {
    if (!opensslHelper.Initialize()) {
        std::cout << "Failed to initialize OpenSSL." << std::endl;
        return INVALID_SOCKET;
    }

    if (this->InitWsa()) {
        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET) {
            std::cout << "Invalid Socket." << std::endl;
            WSACleanup();
            return INVALID_SOCKET;
        }

        SOCKADDR_IN serverAddress;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_addr.s_addr = inet_addr(ip.c_str());
        serverAddress.sin_port = htons(port);

        if (bind(sock, (SOCKADDR *) &serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
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

    while (!terminateProgram) {
        SOCKADDR_IN newClientAddr;
        int newClientLength = sizeof(newClientAddr);
        SOCKET clientSocket = accept(serverSocket, (SOCKADDR *) &newClientAddr, &newClientLength);
        if (clientSocket == INVALID_SOCKET) {
            std::cout << "Accept Error." << std::endl;
            continue;
        }

        SSL *ssl = SSL_new(sslContext);
        SSL_set_fd(ssl, clientSocket);
        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
            closesocket(clientSocket);
            continue;
        }

        std::lock_guard<std::mutex> lock(clientMutex);
        ClientPacket clientpacket;
        clientpacket.ssl = ssl;
        clientpacket.ip = inet_ntoa(newClientAddr.sin_addr);
        clientpacket.port = ntohs(newClientAddr.sin_port);
        clientpacket.isconnected = true;
        connections.push_back(clientpacket);

        std::thread recvThread(&NewServer::ReceiveMessage, this, ssl);
        recvThread.detach();

        std::cout << "New Host connected, IP " << inet_ntoa(newClientAddr.sin_addr) << ", port "
                  << ntohs(newClientAddr.sin_port) << " With ID : " << connections.size() - 1 << std::endl;
    }
}

void NewServer::Cleanup() {
    terminateProgram = true;
    for (int i = 0; i < connections.size(); i++) {
        if (connections[i].ssl) {
            SSL_shutdown(connections[i].ssl);
            SSL_free(connections[i].ssl);
        }
    }
    connections.clear();
    if (sslContext) {
        SSL_CTX_free(sslContext);
    }
    opensslHelper.Cleanup();
    closesocket(serverSocket);
    WSACleanup();
    exit(0);
}