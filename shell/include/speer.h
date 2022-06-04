#ifndef SHELLPEER_H
#define SHELLPEER_H

#include "lsodium.h"
#include "lpeer.h"

typedef struct sShell_peer {
    uint8_t pub[crypto_kx_PUBLICKEYBYTES];
    char hostname[LAIKA_HOSTNAME_LEN], inet[LAIKA_INET_LEN], ipStr[LAIKA_IPSTR_LEN];
    PEERTYPE type;
    OSTYPE osType;
} tShell_peer;

tShell_peer *shellP_newPeer(PEERTYPE type, OSTYPE osType, uint8_t *pub, char *hostname, char *inet, char *ipStr);
void shellP_freePeer(tShell_peer *peer);

void shellP_printInfo(tShell_peer *peer);

#endif