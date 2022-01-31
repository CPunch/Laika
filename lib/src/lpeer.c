#include "lerror.h"
#include "lmem.h"
#include "lpeer.h"

struct sLaika_peer *laikaS_newPeer(PeerPktHandler *handlers, LAIKAPKT_SIZE *pktSizeTable, struct sLaika_pollList *pList, void *uData) {
    struct sLaika_peer *peer = laikaM_malloc(sizeof(struct sLaika_peer));

    laikaS_initSocket(&peer->sock);
    peer->handlers = handlers;
    peer->pktSizeTable = pktSizeTable;
    peer->pList = pList;
    peer->uData = uData;
    peer->priv = NULL;
    peer->pub = NULL;
    peer->pktSize = 0;
    peer->pktID = LAIKAPKT_MAXNONE;
    peer->setPollOut = false;
    return peer;
}

void laikaS_setKeys(struct sLaika_peer *peer, uint8_t *priv, uint8_t *pub) {
    peer->priv = priv;
    peer->pub = pub;
}

void laikaS_freePeer(struct sLaika_peer *peer) {
    laikaS_cleanSocket(&peer->sock);
    laikaM_free(peer);
}

bool laikaS_handlePeerIn(struct sLaika_peer *peer) {
    RAWSOCKCODE err;
    int recvd;

    switch (peer->pktID) {
        case LAIKAPKT_MAXNONE:
            /* try grabbing pktID */
            if (laikaS_rawRecv(&peer->sock, sizeof(uint8_t), &recvd) != RAWSOCK_OK)
                return false;

            peer->pktID = laikaS_readByte(&peer->sock);

            /* sanity check packet ID */
            if (peer->pktID >= LAIKAPKT_MAXNONE)
                LAIKA_ERROR("received evil pktID!\n")

            peer->pktSize = peer->pktSizeTable[peer->pktID];

            if (peer->pktSize == 0)
                LAIKA_ERROR("unsupported packet!\n")

            break;
        case LAIKAPKT_VARPKT_REQ:
            /* try grabbing pktID & size */
            if (laikaS_rawRecv(&peer->sock, sizeof(uint8_t) + sizeof(LAIKAPKT_SIZE), &recvd) != RAWSOCK_OK)
                return false;

            if (recvd != sizeof(uint8_t) + sizeof(LAIKAPKT_SIZE))
                LAIKA_ERROR("couldn't read whole LAIKAPKT_VARPKT_REQ")

            /* read pktID */
            peer->pktID = laikaS_readByte(&peer->sock);

            /* sanity check pktID, (check valid range, check it's variadic) */
            if (peer->pktID >= LAIKAPKT_MAXNONE || peer->pktSizeTable[peer->pktID])
                LAIKA_ERROR("received evil pktID!\n")

            /* try reading new packet size */
            laikaS_readInt(&peer->sock, (void*)&peer->pktSize, sizeof(LAIKAPKT_SIZE));

            if (peer->pktSize > LAIKA_MAX_PKTSIZE)
                LAIKA_ERROR("variable packet too large!")
            break;
        default:
            /* try grabbing the rest of the packet */
            if (laikaS_rawRecv(&peer->sock, peer->pktSize - peer->sock.inCount, &recvd) != RAWSOCK_OK)
                return false;

            /* have we received the full packet? */
            if (peer->pktSize == peer->sock.inCount) {
                PeerPktHandler hndlr = peer->handlers[peer->pktID];

                if (hndlr != NULL) {
                    hndlr(peer, peer->pktID, peer->uData); /* dispatch to packet handler */
                } else
                    LAIKA_ERROR("peer %x doesn't support packet id [%d]!\n", peer, peer->pktID); 

                /* reset */
                peer->sock.inCount = 0;
                peer->pktID = LAIKAPKT_MAXNONE;
            }

            break;
    }

    if (peer->sock.outCount > 0 && !laikaS_handlePeerOut(peer))
        return false;

    return laikaS_isAlive((&peer->sock));
}

bool laikaS_handlePeerOut(struct sLaika_peer *peer) {
    RAWSOCKCODE err;
    int sent;

    if (peer->sock.outCount == 0) /* sanity check */
        return true;

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