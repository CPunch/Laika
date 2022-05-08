#ifndef LAIKA_CNC_PEER_H
#define LAIKA_CNC_PEER_H

#include "laika.h"
#include "lpacket.h"
#include "lsocket.h"
#include "lpolllist.h"
#include "lpeer.h"

struct sLaika_peerInfo {
    struct sLaika_cnc *cnc;
    long lastPing;
    bool completeHandshake;
};

#define BASE_PEERINFO struct sLaika_peerInfo info;

struct sLaika_botInfo {
    BASE_PEERINFO
    struct sLaika_peer *shellAuths[LAIKA_MAX_SHELLS]; /* currently connected shells */
};

struct sLaika_authInfo {
    BASE_PEERINFO
    struct sLaika_peer *shellBot; /* currently connected shell */
    uint32_t shellID;
};

#undef BASE_PEERINFO

struct sLaika_botInfo *laikaC_newBotInfo(struct sLaika_cnc *cnc);
struct sLaika_authInfo *laikaC_newAuthInfo(struct sLaika_cnc *cnc);
void laikaC_freePeerInfo(struct sLaika_peer *peer, struct sLaika_peerInfo *pInfo);

int laikaC_addShell(struct sLaika_botInfo *bInfo, struct sLaika_peer *auth);
void laikaC_rmvShell(struct sLaika_botInfo *bInfo, struct sLaika_peer *auth);

void laikaC_closeBotShells(struct sLaika_peer *bot);

void laikaC_handleHandshakeRequest(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData);
void laikaC_handlePing(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData);
void laikaC_handleShellClose(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData);
void laikaC_handleShellData(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData);

#endif