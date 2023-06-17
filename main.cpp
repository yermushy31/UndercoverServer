#pragma comment(lib, "ws2_32.lib")
#include <iostream>
#include <string>
#include <WS2tcpip.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

class Client {
public:
    Client(const std::string& serverIP, int serverPort)
            : serverIP(serverIP), serverPort(serverPort), clientSocket(INVALID_SOCKET), sslContext(nullptr), sslSocket(nullptr) {}

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

        // Initialize OpenSSL
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();

        // Create SSL context
        const SSL_METHOD* method = TLS_client_method(); // Use SSLv23 method for compatibility
        sslContext = SSL_CTX_new(method);
        if (!sslContext) {
            std::cout << "Failed to create SSL context." << std::endl;
            closesocket(clientSocket);
            WSACleanup();
            return false;
        }

        // Attach SSL socket to the SSL context
        sslSocket = SSL_new(sslContext);
        if (!sslSocket) {
            std::cout << "Failed to create SSL socket." << std::endl;
            SSL_CTX_free(sslContext);
            closesocket(clientSocket);
            WSACleanup();
            return false;
        }
        SSL_set_fd(sslSocket, clientSocket);

        // Perform SSL handshake
        if (SSL_connect(sslSocket) != 1) {
            std::cout << "Failed to perform SSL handshake." << std::endl;
            SSL_free(sslSocket);
            SSL_CTX_free(sslContext);
            closesocket(clientSocket);
            WSACleanup();
            return false;
        }

        return true;
    }

    void Run() {
        std::string input;
        char buffer[4096];
        int bytesRead;

        while (true) {
            bytesRead = SSL_read(sslSocket, buffer, sizeof(buffer) - 1);
            if (bytesRead <= 0) {
                std::cout << "Failed to receive data from the server." << std::endl;
                break;
            }

            buffer[bytesRead] = '\0';
            std::cout << "Server response: " << buffer << std::endl;
        }

        SSL_shutdown(sslSocket);
        SSL_free(sslSocket);
        SSL_CTX_free(sslContext);
        closesocket(clientSocket);
        WSACleanup();
    }

private:
    std::string serverIP;
    int serverPort;
    SOCKET clientSocket;
    SSL_CTX* sslContext;
    SSL* sslSocket;
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