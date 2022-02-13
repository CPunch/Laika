#ifndef LAIKA_CNC_PANEL_H
#define LAIKA_CNC_PANEL_H

#include "lpeer.h"

void laikaC_sendNewPeer(struct sLaika_peer *panel, struct sLaika_peer *bot);
void laikaC_sendRmvPeer(struct sLaika_peer *panel, struct sLaika_peer *bot);
void laikaC_handleAuthenticatedHandshake(struct sLaika_peer *panel, LAIKAPKT_SIZE sz, void *uData);

#endif