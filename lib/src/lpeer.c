#include "lerror.h"
#include "lmem.h"
#include "lpeer.h"

struct sLaika_peer *laikaS_newPeer(void (*pktHandler)(struct sLaika_peer *peer, LAIKAPKT_ID id), struct sLaika_pollList *pList, size_t *pktSizeTable) {
    struct sLaika_peer *peer = laikaM_malloc(sizeof(struct sLaika_peer));

    laikaS_initSocket(&peer->sock);
    peer->pktHandler = pktHandler;
    peer->pList = pList;
    peer->pktSizeTable = pktSizeTable;
    peer->pktSize = 0;
    peer->pktID = LAIKAPKT_MAXNONE;
    peer->setPollOut = false;
    return peer;
}

void laikaS_freePeer(struct sLaika_peer *peer) {
    laikaS_cleanSocket(&peer->sock);
    laikaM_free(peer);
}

bool laikaS_handlePeerIn(struct sLaika_peer *peer) {
    RAWSOCKCODE err;
    int recvd;
    bool _tryCatchRes;

    switch (peer->pktID) {
        case LAIKAPKT_MAXNONE:
            /* try grabbing pktID */
            if (laikaS_rawRecv(&peer->sock, sizeof(uint8_t), &recvd) != RAWSOCK_OK)
                return false;

            peer->pktID = laikaS_readByte(&peer->sock);

            /* sanity check packet ID */
            if (peer->pktID >= LAIKAPKT_MAXNONE)
                LAIKA_ERROR("received evil pktID!")

            peer->pktSize = peer->pktSizeTable[peer->pktID];
            break;
        default:
            /* try grabbing the rest of the packet */
            if (laikaS_rawRecv(&peer->sock, peer->pktSize - peer->sock.inCount, &recvd) != RAWSOCK_OK)
                return false;

            /* have we received the full packet? */
            if (peer->pktSize == peer->sock.inCount) {
                /* dispatch to packet handler */
                LAIKA_TRY
                    peer->pktHandler(peer, peer->pktID);
                    _tryCatchRes = true;
                LAIKA_CATCH
                    _tryCatchRes = false;
                LAIKA_TRYEND /* can't skip this, so the return is after */

                return _tryCatchRes;
            }
    }
}

bool laikaS_handlePeerOut(struct sLaika_peer *peer) {
    RAWSOCKCODE err;
    int sent;

    if (peer->sock.outCount == 0) /* sanity check */
        return;

    switch (laikaS_rawSend(&peer->sock, peer->sock.outCount, &sent)) {
        case RAWSOCK_OK: /* we're ok! */
            if (peer->setPollOut) { /* if POLLOUT was set, unset it */
                laikaP_rmvPollOut(peer->pList, &peer->sock);
                peer->setPollOut = false;
            }
            return true;
        case RAWSOCK_POLL: /* we've been asked to set the POLLOUT flag */
            if (!peer->setPollOut) { /* if POLLOUT wasn't set, set it so we'll be notified whenever the kernel has room :) */
                laikaP_addPollOut(peer->pList, &peer->sock);
                peer->setPollOut = true;
            }
            return true;
        default: /* panic! */
        case RAWSOCK_CLOSED:
        case RAWSOCK_ERROR:
            return false;
    }

}