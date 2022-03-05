#ifndef SHELLPEER_H
#define SHELLPEER_H

#include "lsodium.h"
#include "lpeer.h"

typedef struct sShell_peer {
    uint8_t pub[crypto_kx_PUBLICKEYBYTES];
    char hostname[LAIKA_HOSTNAME_LEN], inet[LAIKA_INET_LEN], ipv4[LAIKA_IPV4_LEN];
    PEERTYPE type;
} tShell_peer;

tShell_peer *shellP_newPeer(PEERTYPE type, uint8_t *pub, char *hostname, char *inet, char *ipv4);
void shellP_freePeer(tShell_peer *peer);

char *shellP_typeStr(tShell_peer *peer);

#endif