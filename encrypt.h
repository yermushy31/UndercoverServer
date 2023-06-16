//
// Created by yermushy on 16/06/2023.
// serversocket

#ifndef UNDERCOVERSERVER_ENCRYPT_H
#define UNDERCOVERSERVER_ENCRYPT_H
#include <iostream>
#include <WinSock2.h>
#include <vector>
#include <openssl/ssl.h>
#include <openssl/err.h>

class Encryption {
public:
    Encryption(SOCKET clientsock, SOCKET serversock);
    ~Encryption();
    void CheckSslVersion(void);
    void InitSSL(void);
    bool CreateSSLContext();
    bool LoadSSLCertificate();
    bool LoadSSLPrivateKey();
    bool CreateSSLObject();
    bool BindSSL();
    bool SSLHandshake();
    void SendSSL(const char* buffer);
private:
    SSL* ssl;
    SSL_CTX* SSLContext = nullptr;
    const char* Certificate = nullptr;
    const char* PrivateKey = nullptr;
    SOCKET Clientsock;
    SOCKET Serversock;

};
#endif //UNDERCOVERSERVER_ENCRYPT_H
