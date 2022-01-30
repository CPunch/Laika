#include <stdio.h>
#include <string.h>

#include "lerror.h"
#include "lrsa.h"

#define DATA "Encryption/Decryption test passed!\n"
#define DATALEN 36
#define CIPHERLEN crypto_box_SEALBYTES + DATALEN

int main(int argv, char **argc) {
    unsigned char priv[crypto_box_SECRETKEYBYTES], pub[crypto_box_PUBLICKEYBYTES];
    char buf[256];
    unsigned char enc[CIPHERLEN];
    unsigned char dec[DATALEN];

    if (sodium_init() < 0) {
        printf("Libsodium failed to init!\n");
        return 1;
    }

    crypto_box_keypair(pub, priv);

    printf("[~] Generated keypair!\n");
    sodium_bin2hex(buf, 256, pub, crypto_box_PUBLICKEYBYTES);
    printf("[~] public key: %s\n", buf);
    sodium_bin2hex(buf, 256, priv, crypto_box_SECRETKEYBYTES);
    printf("[~] private key: %s\n\n", buf);

    if (crypto_box_seal(enc, DATA, DATALEN, pub) != 0) {
        printf("Failed to encrypt!\n");
        return 1;
    }

    if (crypto_box_seal_open(dec, enc, CIPHERLEN, pub, priv) != 0) {
        printf("Failed to decrypt!\n");
        return 1;
    }

    printf("%s", dec);
    return 0;
}