#ifndef SHELLCLIENT_H
#define SHELLCLIENT_H

#include "hashmap.h"
#include "lpeer.h"
#include "lsodium.h"

#include "speer.h"

typedef struct sShell_client {
    uint8_t priv[crypto_kx_SECRETKEYBYTES], pub[crypto_kx_PUBLICKEYBYTES];
    struct sLaika_pollList pList;
    struct sLaika_peer *peer;
    struct hashmap *peers;
    tShell_peer **peerTbl;
    int peerTblCount;
    int peerTblCap;
} tShell_client;

void shellC_init(tShell_client *client);
void shellC_cleanup(tShell_client *client);

void shellC_connectToCNC(tShell_client *client, char *ip, char *port);
bool shellC_poll(tShell_client *client, int timeout);

tShell_peer *shellC_getPeerByPub(tShell_client *client, uint8_t *pub, int *id);

int shellC_addPeer(tShell_client *client, tShell_peer *peer); /* returns new peer id */
void shellC_rmvPeer(tShell_client *client, tShell_peer *peer, int id);

void shellC_printInfo(tShell_peer *peer);

#endif