#ifndef LAIKA_CNC_H
#define LAIKA_CNC_H

#include "laika.h"
#include "lpacket.h"
#include "lsocket.h"
#include "lpolllist.h"
#include "lpeer.h"

struct sLaika_cnc {
    uint8_t priv[crypto_kx_SECRETKEYBYTES], pub[crypto_kx_PUBLICKEYBYTES];
    struct sLaika_socket sock;
    struct sLaika_pollList pList;
    struct sLaika_peer **panels; /* holds connected panel peers */
    int panelCount;
    int panelCap;
};

struct sLaika_cnc *laikaC_newCNC(uint16_t port);
void laikaC_freeCNC(struct sLaika_cnc *cnc);

void laikaC_onAddPeer(struct sLaika_cnc *cnc, struct sLaika_peer *peer);
void laikaC_onRmvPeer(struct sLaika_cnc *cnc, struct sLaika_peer *peer);

void laikaC_addPanel(struct sLaika_cnc *cnc, struct sLaika_peer *panel);
void laikaC_rmvPanel(struct sLaika_cnc *cnc, struct sLaika_peer *panel);

void laikaC_killPeer(struct sLaika_cnc *cnc, struct sLaika_peer *peer);
bool laikaC_pollPeers(struct sLaika_cnc *cnc, int timeout);

#endif