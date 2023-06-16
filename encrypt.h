//
// Created by yermushy on 16/06/2023.
//

#ifndef UNDERCOVERCLIENT_ENCRYPT_H
#define UNDERCOVERCLIENT_ENCRYPT_H


#include <iostream>

#include <openssl/ssl.h>
#include <openssl/err.h>

class Encryption {
public:
    void CheckSslVersion(void);
};


#endif //UNDERCOVERCLIENT_ENCRYPT_H
