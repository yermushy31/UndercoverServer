#include <iostream>
#include <string>
#include <thread>
#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

class Client {
private:
    SOCKET clientSocket;
    SOCKADDR_IN serverAddress;
    char recvBuffer[4096];
public:
    Client() {
        // Initialize Winsock
        WSADATA wsData;
        if (WSAStartup(MAKEWORD(2, 2), &wsData) != 0) {
            std::cerr << "Failed to initialize Winsock." << std::endl;
            exit(1);
        }
        // Create client socket
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Failed to create client socket." << std::endl;
            WSACleanup();
            exit(1);
        }

        // Set server address
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(5555);
        serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    }

    ~Client() {
        closesocket(clientSocket);
        WSACleanup();
    }

    void connectToServer() {
        // Connect to the server
        if (connect(clientSocket, reinterpret_cast<SOCKADDR*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
            std::cerr << "Failed to connect to the server." << std::endl;
            closesocket(clientSocket);
            WSACleanup();
            exit(1);
        }
    }

    void receiveMessage() {
        int bytesRead;
        do {
            bytesRead = recv(clientSocket, recvBuffer, sizeof(recvBuffer) - 1, 0);
            if (bytesRead > 0) {
                recvBuffer[bytesRead] = '\0';
                //letsgooooooooooooooooooooooooo
                std::cout << recvBuffer << std::endl;
            }
            else if (bytesRead == 0) {
                std::cout << "Connection closed by the server." << std::endl;
            }
            else {
                std::cerr << "Receive error." << std::endl;
            }
        } while (bytesRead > 0);
    }

    void start() {
        connectToServer();
        std::thread recvThread(&Client::receiveMessage, this);
        recvThread.join();
    }
};

int main() {
    Client client;
    client.start();

    return 0;
}