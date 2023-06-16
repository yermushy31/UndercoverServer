#pragma comment(lib, "ws2_32.lib")

#include <iostream>
#include <string>
#include <WS2tcpip.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

/*https://reversea.me/index.php/hybrid-encryption-sockets-using-crypto/ */

class Client {
public:
    Client(const std::string& serverIP, int serverPort)
            : serverIP(serverIP), serverPort(serverPort), clientSocket(INVALID_SOCKET) {}

    bool Connect() {
        WSADATA wsData;
        if (WSAStartup(MAKEWORD(2, 2), &wsData) != 0) {
            std::cout << "Failed to initialize Winsock." << std::endl;
            return false;
        }

        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == INVALID_SOCKET) {
            std::cout << "Failed to create socket." << std::endl;
            WSACleanup();
            return false;
        }

        sockaddr_in serverAddress{};
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(serverPort);
        if (inet_pton(AF_INET, serverIP.c_str(), &(serverAddress.sin_addr)) <= 0) {
            std::cout << "Invalid address or address not supported." << std::endl;
            closesocket(clientSocket);
            WSACleanup();
            return false;
        }

        if (connect(clientSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
            std::cout << "Failed to connect to the server." << std::endl;
            closesocket(clientSocket);
            WSACleanup();
            return false;
        }

        return true;
    }

    void Run() {
        std::cout << "Connected to the server. Enter 'exit' to quit." << std::endl;

        std::string input;
        char buffer[4096];
        int bytesRead;

        while (true) {
            bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            if (bytesRead == SOCKET_ERROR) {
                std::cout << "Failed to receive data from the server." << std::endl;
                break;
            }

            buffer[bytesRead] = '\0';
            std::cout << "Server response: " << buffer << std::endl;
        }

        closesocket(clientSocket);
        WSACleanup();
    }

private:
    std::string serverIP;
    int serverPort;
    SOCKET clientSocket;
};

int main() {
    std::string serverIP = "127.0.0.1";
    int serverPort = 5555;

    Client client(serverIP, serverPort);
    if (client.Connect()) {
        client.Run();
    }

    return 0;
}