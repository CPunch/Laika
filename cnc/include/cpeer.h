#ifndef LAIKA_CNC_PEER_H
#define LAIKA_CNC_PEER_H

#include "laika.h"
#include "lpacket.h"
#include "lpeer.h"
#include "lpolllist.h"
#include "lsocket.h"

struct sLaika_peerInfo
{
    struct sLaika_shellInfo *shells[LAIKA_MAX_SHELLS]; /* currently connected shells */
    struct sLaika_cnc *cnc;
    long lastPing;
    bool completeHandshake;
};

struct sLaika_shellInfo
{
    struct sLaika_peer *bot;
    struct sLaika_peer *auth;
    uint32_t botShellID, authShellID;
    uint16_t cols, rows;
};

#define BASE_PEERINFO struct sLaika_peerInfo info;

/* these will differ someday */
struct sLaika_botInfo
{
    BASE_PEERINFO
};

struct sLaika_authInfo
{
    BASE_PEERINFO
};

#undef BASE_PEERINFO

#define GETPINFOFROMPEER(x) ((struct sLaika_peerInfo *)x->uData)
#define GETBINFOFROMPEER(x) ((struct sLaika_botInfo *)x->uData)
#define GETAINFOFROMPEER(x) ((struct sLaika_authInfo *)x->uData)

struct sLaika_botInfo *laikaC_newBotInfo(struct sLaika_cnc *cnc);
struct sLaika_authInfo *laikaC_newAuthInfo(struct sLaika_cnc *cnc);
void laikaC_freePeerInfo(struct sLaika_peer *peer, struct sLaika_peerInfo *pInfo);

struct sLaika_shellInfo *laikaC_openShell(struct sLaika_peer *bot, struct sLaika_peer *auth,
                                          uint16_t cols, uint16_t rows);
void laikaC_closeShell(struct sLaika_shellInfo *shell);

void laikaC_closeShells(struct sLaika_peer *peer);

void laikaC_handleHandshakeRequest(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData);
void laikaC_handlePeerLoginReq(struct sLaika_peer *authPeer, LAIKAPKT_SIZE sz, void *uData);
void laikaC_handlePing(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData);
void laikaC_handleShellClose(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData);
void laikaC_handleShellData(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData);

#endif