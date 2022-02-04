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
    peer->pktSize = 0;
    peer->type = PEER_UNVERIFIED;
    peer->pktID = LAIKAPKT_MAXNONE;
    peer->setPollOut = false;
    peer->outStart = -1;
    peer->inStart = -1;
    peer->useSecure = false;
    return peer;
}

void laikaS_freePeer(struct sLaika_peer *peer) {
    laikaS_cleanSocket(&peer->sock);
    laikaM_free(peer);
}

void laikaS_startOutPacket(struct sLaika_peer *peer, uint8_t id) {
    struct sLaika_socket *sock = &peer->sock;

    if (peer->outStart != -1) { /* sanity check */
        LAIKA_ERROR("unended OUT packet!\n")
    }
    laikaS_writeByte(sock, id);

    peer->outStart = sock->outCount;
    if (peer->useSecure) { /* if we're encrypting this packet, append the nonce right after the packet ID */
        uint8_t nonce[crypto_secretbox_NONCEBYTES];
        randombytes_buf(nonce, crypto_secretbox_NONCEBYTES);
        laikaS_write(sock, nonce, crypto_secretbox_NONCEBYTES);
    }
}

int laikaS_endOutPacket(struct sLaika_peer *peer) {
    struct sLaika_socket *sock = &peer->sock;
    uint8_t *body;
    size_t sz;

    if (peer->useSecure) {
        /* make sure we have enough space */
        laikaM_growarray(uint8_t, sock->outBuf, crypto_secretbox_MACBYTES, sock->outCount, sock->outCap);

        /* packet body starts after the id & nonce */
        body = &sock->outBuf[peer->outStart + crypto_secretbox_NONCEBYTES];
        /* encrypt packet body in-place */
        if (crypto_secretbox_easy(body, body, (sock->outCount - peer->outStart) - crypto_secretbox_NONCEBYTES,
                &sock->outBuf[peer->outStart], peer->outKey) != 0) {
            LAIKA_ERROR("Failed to encrypt packet!\n")
        }

        sock->outCount += crypto_secretbox_MACBYTES;
    }

    sz = sock->outCount - peer->outStart;
    peer->outStart = -1;
    return sz;
}

void laikaS_startInPacket(struct sLaika_peer *peer) {
    struct sLaika_socket *sock = &peer->sock;

    if (peer->inStart != -1) { /* sanity check */
        LAIKA_ERROR("unended IN packet!\n")
    }

    peer->inStart = sock->inCount;
}

int laikaS_endInPacket(struct sLaika_peer *peer) {
    struct sLaika_socket *sock = &peer->sock;
    uint8_t *body;
    size_t sz = sock->inCount - peer->inStart;

    if (peer->useSecure) {
        body = &sock->inBuf[peer->inStart + crypto_secretbox_NONCEBYTES];

        /* decrypt packet body in-place */
        if (crypto_secretbox_open_easy(body, body, (sock->inCount - peer->inStart) - crypto_secretbox_NONCEBYTES,
                &sock->inBuf[peer->inStart], peer->inKey) != 0) {
            LAIKA_ERROR("Failed to decrypt packet!\n")
        }

        /* decrypted message is smaller now */
        sock->inCount -= crypto_secretbox_MACBYTES;

        /* remove nonce */
        laikaM_rmvarray(sock->inBuf, sock->inCount, peer->inStart, crypto_secretbox_NONCEBYTES);

        sz -= crypto_secretbox_MACBYTES + crypto_secretbox_NONCEBYTES;
    }

    peer->inStart = -1;
    return sz;
}

void laikaS_setSecure(struct sLaika_peer *peer, bool flag) {
    peer->useSecure = flag;
}

bool laikaS_handlePeerIn(struct sLaika_peer *peer) {
    RAWSOCKCODE err;
    int recvd;

    switch (peer->pktID) {
        case LAIKAPKT_MAXNONE:
            /* try grabbing pktID */
            if (laikaS_rawRecv(&peer->sock, sizeof(uint8_t), &recvd) != RAWSOCK_OK)
                return false;

            /* read packet ID & mark start of packet */
            peer->pktID = laikaS_readByte(&peer->sock);
            laikaS_startInPacket(peer);

            /* sanity check packet ID */
            if (peer->pktID >= LAIKAPKT_MAXNONE)
                LAIKA_ERROR("received evil pktID!\n")

            peer->pktSize = peer->pktSizeTable[peer->pktID];

            if (peer->pktSize == 0)
                LAIKA_ERROR("unsupported packet!\n")

            /* if we're encrypting/decrypting all packets, make sure to make the packetsize reflect this */
            if (peer->useSecure)
                peer->pktSize += crypto_secretbox_MACBYTES + crypto_secretbox_NONCEBYTES;

            break;
        //case LAIKAPKT_VARPKT_REQ:
            /* try grabbing pktID & size */
        //    if (laikaS_rawRecv(&peer->sock, sizeof(uint8_t) + sizeof(LAIKAPKT_SIZE), &recvd) != RAWSOCK_OK)
        //        return false;

        //    if (recvd != sizeof(uint8_t) + sizeof(LAIKAPKT_SIZE))
        //        LAIKA_ERROR("couldn't read whole LAIKAPKT_VARPKT_REQ")

            /* read pktID */
        //    peer->pktID = laikaS_readByte(&peer->sock);

            /* sanity check pktID, (check valid range, check it's variadic) */
        //    if (peer->pktID >= LAIKAPKT_MAXNONE || peer->pktSizeTable[peer->pktID])
        //        LAIKA_ERROR("received evil pktID!\n")

            /* try reading new packet size */
        //    laikaS_readInt(&peer->sock, (void*)&peer->pktSize, sizeof(LAIKAPKT_SIZE));

        //    if (peer->pktSize > LAIKA_MAX_PKTSIZE)
        //        LAIKA_ERROR("variable packet too large!")
        //    break;
        default:
            /* try grabbing the rest of the packet */
            if (laikaS_rawRecv(&peer->sock, peer->pktSize - peer->sock.inCount, &recvd) != RAWSOCK_OK)
                return false;

            /* have we received the full packet? */
            if (peer->pktSize == peer->sock.inCount) {
                PeerPktHandler hndlr = peer->handlers[peer->pktID];
                peer->pktSize = laikaS_endInPacket(peer);

                if (hndlr != NULL) {
                    hndlr(peer, peer->pktSize, peer->uData); /* dispatch to packet handler */
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