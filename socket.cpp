#include "socket.h"


bool NewServer::InitWsa() {
    if (WSAStartup(dllVersion, &wsadata) != 0) {
        std::cout << "Error WSAstartup " << std::endl;
    }
    return true;
}

void NewServer::DisplayClients() {
    std::lock_guard<std::mutex> lock(clientMutex);
    for (size_t i = 0; i < connections.size(); i++) {
        std::cout << "[" << i << "] Client-" << i << std::endl;
    }
    std::cout << "Which client do you want to connect to?" << std::endl;
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
        res = recv(currentClient, recvBuffer, sizeof(recvBuffer), 0);
        if (res == SOCKET_ERROR) {
            std::cout << "RECV error on thread " << currentThreadId << std::endl;
        }
        std::cout << "Client number [" << connections.size() - 1 << "] Said: " << std::endl;
        std::cout << recvBuffer << std::endl;
    } while (res > 0);
}

void NewServer::SendAll(int id, const char* data, int totalBytes) {

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
            std::cout << "server~#";
        }
        std::getline(std::cin, inputStr);
        if (inputStr == "connect") {
            DisplayClients();
            int id;
            std::cin >> id;
            std::cin.ignore();  // Discard the newline character
            clientMode = true;
            while (clientMode) {
                memset(sendBuffer, 0x0, sizeof(sendBuffer));
                std::cout << "Client~" << id << "#" << std::endl;
                std::getline(std::cin, inputStr);

                if (inputStr.find("exit") != std::string::npos) {
                    id = 0;
                    clientMode = false;
                    break;
                }
                SendAll(id, inputStr.c_str(), inputStr.size());
            }
        }
        else if (inputStr == "clear") {
            system("cls");
        }
    }
}

SOCKET NewServer::CreateNewSocket(const std::string& ip, int port) {
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

    while (true) {
        SOCKADDR_IN newClientAddr;
        int newClientLength = sizeof(newClientAddr);
        SOCKET clientSocket = accept(serverSocket, (SOCKADDR*)&newClientAddr, &newClientLength);
        if (clientSocket == INVALID_SOCKET) {
            std::cout << "Accept Error." << std::endl;
            CloseConnection(serverSocket);
            return;
        }


        std::lock_guard<std::mutex> lock(clientMutex);
        connections.push_back(clientSocket);


        std::thread recvThread(&NewServer::ReceiveMessage, this, clientSocket);
        recvThread.detach();

        std::cout << "New Host connected, IP " << inet_ntoa(newClientAddr.sin_addr) << ", port " << ntohs(newClientAddr.sin_port) << std::endl;
    }
}

NewServer::~NewServer() {
    closesocket(serverSocket);
    WSACleanup();
}

NewServer::NewServer(const std::string& ip, int port) : clientMode(false), serverSocket(INVALID_SOCKET) {
    serverSocket = CreateNewSocket(ip, port);
}