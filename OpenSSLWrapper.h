//
// Created by vecto on 17/06/2023.
//

#ifndef UNDERCOVERSERVER_OPENSSLWRAPPER_H
#define UNDERCOVERSERVER_OPENSSLWRAPPER_H
#include <openssl/ssl.h>
#include <openssl/err.h>

#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")

class OpenSSLHelper {
public:
    OpenSSLHelper();
    ~OpenSSLHelper();
    bool Initialize();
    void Cleanup();
    SSL_CTX* CreateContext();
    void ConfigureContext(SSL_CTX* ctx);

private:
    SSL_CTX* context;
};

#endif //UNDERCOVERSERVER_OPENSSLWRAPPER_H
