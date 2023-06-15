#include "socket.h"


bool NewServer::InitWsa() {
    if (WSAStartup(dllVersion, &wsadata) != 0) {
        std::cout << "Error WSAstartup " << std::endl;
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
    } else {
        std::cout << "No clients is connected at the moment" << std::endl;
    }

}

void NewServer::CloseConnection(SOCKET s) {
    closesocket(s);
    WSACleanup();
}

void NewServer::ReceiveMessage(SOCKET currentClient) {
    std::thread::id currentThreadId = std::this_thread::get_id();
    std::cout << "New Thread Created with the ID: " << currentThreadId << std::endl;
    std::cout << "Client number " << connections.size() - 1 << " got the ID " << currentThreadId << std::endl;
    int res;
    do {
        res = recv(currentClient, recvBuffer, sizeof(recvBuffer) - 1, 0);
        if (res == SOCKET_ERROR)
            std::cout << "RECV error on thread " << currentThreadId << std::endl;
        if (res > 0) {

            recvBuffer[res] = '\0';
            std::cout << "Client number [" << connections.size() - 1 << "] Said: " << recvBuffer << std::endl;
        }
    } while (res > 0);
}

void NewServer::SendAll(int id, const char *data, int totalBytes) {

    std::lock_guard<std::mutex> lock(clientMutex);
    int bytesSent = 0;
    while (bytesSent < totalBytes) {
        int bytesRemaining = totalBytes - bytesSent;
        int bytesToSend = (bytesRemaining < 4096) ? bytesRemaining : 4096;
        if (send(connections[id], data + bytesSent, bytesToSend, 0) == SOCKET_ERROR) {
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

            //DisplayClients();
            // Discard the newline character

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
                    } else {
                        std::cout << "Invalid client ID." << std::endl;
                    }
                } catch (const std::exception &e) {
                    std::cout << "Invalid client ID format. Please enter a valid integer." << std::endl;
                }
            } else if (inputStr == "clear") {
                system("cls");
            }
        }
    }
}

SOCKET NewServer::CreateNewSocket(const std::string &ip, int port) {
    WSADATA wsadata;
    WORD dllVersion = MAKEWORD(2, 2);
    if (WSAStartup(dllVersion, &wsadata) != 0) {
        std::cout << "Error WSAStartup " << std::endl;
        return INVALID_SOCKET;
    }

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

    std::cout << "Listening..." << std::endl;
    return sock;
}

void NewServer::StartServer() {
    inputThread = std::thread(&NewServer::HandleInput, this);
    inputThread.detach();

    while (true) {
        SOCKADDR_IN newClientAddr;
        int newClientLength = sizeof(newClientAddr);
        SOCKET clientSocket = accept(serverSocket, (SOCKADDR *) &newClientAddr, &newClientLength);
        if (clientSocket == INVALID_SOCKET) {
            std::cout << "Accept Error." << std::endl;
            CloseConnection(serverSocket);
            return;
        }


        std::lock_guard<std::mutex> lock(clientMutex);
        connections.push_back(clientSocket);


        std::thread recvThread(&NewServer::ReceiveMessage, this, clientSocket);
        recvThread.detach();

        std::cout << "New Host connected, IP " << inet_ntoa(newClientAddr.sin_addr) << ", port "
                  << ntohs(newClientAddr.sin_port) << std::endl;
    }
}

NewServer::~NewServer() {
    closesocket(serverSocket);
    WSACleanup();
}

NewServer::NewServer(const std::string &ip, int port) {
    serverSocket = CreateNewSocket(ip, port);
}