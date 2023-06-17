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

