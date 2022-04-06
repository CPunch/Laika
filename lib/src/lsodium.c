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

bool laikaK_checkAuth(uint8_t *pubKey, uint8_t **authKeys, int keys) {
    char buf[128]; /* i don't expect bin2hex to write outside this, but it's only user-info and doesn't break anything (ie doesn't write outside the buffer) */
    int i;

    /* check if key is in authKey list */
    for (i = 0; i < keys; i++) {
        if (sodium_memcmp(pubKey, authKeys[i], crypto_kx_PUBLICKEYBYTES) == 0)
            return true;
    }

    /* key not found */
    return false;
}
