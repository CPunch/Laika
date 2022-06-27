#ifndef LAIKA_CNC_H
#define LAIKA_CNC_H

#include "hashmap.h"
#include "laika.h"
#include "lpacket.h"
#include "lpeer.h"
#include "lpolllist.h"
#include "lsocket.h"
#include "ltask.h"

/* kill peers if they haven't ping'd within a minute */
#define LAIKA_PEER_TIMEOUT 60 * 1000

typedef bool (*tLaika_peerIter)(struct sLaika_peer *peer, void *uData);

struct sLaika_cnc
{
    uint8_t priv[crypto_kx_SECRETKEYBYTES], pub[crypto_kx_PUBLICKEYBYTES];
    struct sLaika_socket sock;
    struct sLaika_pollList pList;
    struct hashmap *peers;          /* holds all peers, lookup using pubkey */
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

/* kills peers who haven't ping'd in a while */
void laikaC_sweepPeersTask(struct sLaika_taskService *service, struct sLaika_task *task,
                           clock_t currTick, void *uData);

void laikaC_iterPeers(struct sLaika_cnc *cnc, tLaika_peerIter iter, void *uData);

#endif