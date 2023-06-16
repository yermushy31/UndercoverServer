//
// Created by yermushy on 16/06/2023.
//

#include "encrypt.h"

void Encryption::CheckSslVersion(void) {
    std::cout << "SSLeay Version: " << SSLeay_version(SSLEAY_VERSION) << std::endl;
    SSL_library_init();
    auto ctx = SSL_CTX_new(SSLv23_client_method());
    if (ctx) {
        auto ssl = SSL_new(ctx);
        if (ssl) {
            std::cout << "SSL Version: " << SSL_get_version(ssl) << std::endl;
            SSL_free(ssl);
        } else {
            std::cout << "SSL_new failed..." << std::endl;
        }
        SSL_CTX_free(ctx);
    } else {
        std::cout << "SSL_CTX_new failed..." << std::endl;
    }
}


void Encryption::InitSSL(void) {
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
}

bool Encryption::CreateSSLContext() {
    this->SSLContext = SSL_CTX_new(TLS_server_method());
    if (SSLContext == nullptr) {
        fprintf(stderr, "SSL context creation failed\n");
        closesocket(this->Clientsock);
        closesocket(this->Serversock);
        return false;
    }
    return true;
}

bool Encryption::LoadSSLCertificate() {
    if (SSL_CTX_use_certificate_file(this->SSLContext, this->Certificate, SSL_FILETYPE_PEM) != 1) {
        fprintf(stderr, "Server certificate loading failed\n");
        SSL_CTX_free(this->SSLContext);
        closesocket(this->Clientsock);
        closesocket(this->Serversock);
        return false;
    }
    return true;
}

bool Encryption::LoadSSLPrivateKey() {
    if (SSL_CTX_use_PrivateKey_file(this->SSLContext, this->PrivateKey, SSL_FILETYPE_PEM) != 1) {
        fprintf(stderr, "Server private key loading failed\n");
        SSL_CTX_free(this->SSLContext);
        closesocket(this->Clientsock);
        closesocket(this->Serversock);
        return false;
    }
    return true;
}

bool Encryption::CreateSSLObject() {
   this->ssl = SSL_new(this->SSLContext);
    if (this->ssl == nullptr) {
        fprintf(stderr, "SSL object creation failed\n");
        SSL_CTX_free(this->SSLContext);
        closesocket(this->Clientsock);
        closesocket(this->Serversock);
        return false;
    }
    return true;
}

bool Encryption::BindSSL() {
    if (SSL_set_fd(this->ssl, this->Clientsock) != 1) {
        fprintf(stderr, "SSL binding failed\n");
        SSL_free(ssl);
        SSL_CTX_free(this->SSLContext);
        closesocket(this->Clientsock);
        closesocket(this->Serversock);
        return false;
    }
    return true;
}

bool Encryption::SSLHandshake() {
    if (SSL_accept(this->ssl) != 1) {
        fprintf(stderr, "SSL handshake failed\n");
        SSL_free(this->ssl);
        SSL_CTX_free(this->SSLContext);
        closesocket(this->Clientsock);
        closesocket(this->Serversock);
        return 1;
    }
}

void Encryption::SendSSL(const char* buffer) {
    SSL_write(this->ssl, buffer, strlen(buffer));
}

Encryption::Encryption(SOCKET clientsock, SOCKET serversock) {
    Clientsock = clientsock;
    Serversock = serversock;
}

Encryption::~Encryption() {
    SSL_shutdown(this->ssl);
    SSL_free(this->ssl);
    SSL_CTX_free(this->SSLContext);
}