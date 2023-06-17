//
// Created by vecto on 17/06/2023.
//
#include "OpenSSLWrapper.h"
OpenSSLHelper::OpenSSLHelper() : context(nullptr) {}

OpenSSLHelper::~OpenSSLHelper() {
    Cleanup();
}

bool OpenSSLHelper::Initialize() {
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    return true;
}

void OpenSSLHelper::Cleanup() {
    ERR_free_strings();
    EVP_cleanup();
}

SSL_CTX* OpenSSLHelper::CreateContext() {
    const SSL_METHOD* method = SSLv23_server_method(); // Use SSLv23 method for compatibility
    SSL_CTX* ctx = SSL_CTX_new(method);
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        return nullptr;
    }
    return ctx;
}

void OpenSSLHelper::ConfigureContext(SSL_CTX* ctx) {
    // Enable automatic ECDH curve selection
    // Load server's certificate and private key
    if (SSL_CTX_use_certificate_file(ctx, "..\\server.crt", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, "..\\server.key", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}
