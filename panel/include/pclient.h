#ifndef PCLIENT_H
#define PCLIENT_H

#include "laika.h"
#include "lpeer.h"
#include "lpolllist.h"
#include "pbot.h"

typedef struct sPanel_client {
    uint8_t priv[crypto_kx_SECRETKEYBYTES], pub[crypto_kx_PUBLICKEYBYTES];
    struct sLaika_pollList pList;
    struct sLaika_peer *peer;
} tPanel_client;

tPanel_client *panelC_newClient();
void panelC_freeClient(tPanel_client *client);

void panelC_connectToCNC(tPanel_client *client, char *ip, char *port); /* can throw a LAIKA_ERROR */
bool panelC_poll(tPanel_client *client, int timeout);

void panelC_addBot(tPanel_bot *bot);
void panelC_rmvBot(tPanel_bot *bot);

#endif
