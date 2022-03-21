#include "lsodium.h"

#include <string.h>

bool laikaK_loadKeys(uint8_t *outPub, uint8_t *outPriv, const char *inPub, const char *inPriv) {
    size_t _unused;

    if (outPub && sodium_hex2bin(outPub, crypto_kx_PUBLICKEYBYTES, inPub, strlen(inPub), NULL, &_unused, NULL) != 0)
        return false; 
    
    if (outPriv && sodium_hex2bin(outPriv, crypto_kx_SECRETKEYBYTES, inPriv, strlen(inPriv), NULL, &_unused, NULL) != 0)
        return false;

    return true;
}

bool laikaK_genKeys(uint8_t *outPub, uint8_t *outPriv) {
    return crypto_kx_keypair(outPub, outPriv) == 0;
}
