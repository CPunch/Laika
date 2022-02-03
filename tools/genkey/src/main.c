#include <stdio.h>
#include <string.h>

#include "lerror.h"
#include "lrsa.h"

int main(int argv, char **argc) {
    unsigned char priv[crypto_kx_SECRETKEYBYTES], pub[crypto_kx_PUBLICKEYBYTES];
    char buf[256];

    if (sodium_init() < 0) {
        printf("Libsodium failed to init!\n");
        return 1;
    }

    crypto_kx_keypair(pub, priv);

    printf("[~] Generated keypair!\n");
    sodium_bin2hex(buf, 256, pub, crypto_kx_PUBLICKEYBYTES);
    printf("[~] public key: %s\n", buf);
    sodium_bin2hex(buf, 256, priv, crypto_kx_SECRETKEYBYTES);
    printf("[~] private key: %s\n\n", buf);

    return 0;
}