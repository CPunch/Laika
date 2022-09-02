#include "lpeer.h"

#include "lerror.h"
#include "lmem.h"

struct sLaika_peer *laikaS_newPeer(struct sLaika_peerPacketInfo *pktTbl,
                                   struct sLaika_pollList *pList, pollFailEvent onPollFail,
                                   void *onPollFailUData, void *uData)
{
    struct sLaika_peer *peer = laikaM_malloc(sizeof(struct sLaika_peer));

    laikaS_initSocket(&peer->sock, laikaS_handlePeerIn, laikaS_handlePeerOut, onPollFail,
                      onPollFailUData);

    peer->packetTbl = pktTbl;
    peer->pList = pList;
    peer->uData = uData;
    peer->pktSize = 0;
    peer->type = PEER_PEER;
    peer->osType = OS_UNKNWN;
    peer->pktID = LAIKAPKT_MAXNONE;
    peer->outStart = -1;
    peer->inStart = -1;
    peer->useSecure = false;

    /* zero-out peer info */
    memset(peer->hostname, 0, LAIKA_HOSTNAME_LEN);
    memset(peer->inet, 0, LAIKA_INET_LEN);
    memset(peer->ipStr, 0, LAIKA_IPSTR_LEN);

    /* generate peer's salt */
    laikaS_genSalt(peer);

    return peer;
}

void laikaS_freePeer(struct sLaika_peer *peer)
{
    laikaS_cleanSocket(&peer->sock);
    laikaM_free(peer);
}

void laikaS_setSalt(struct sLaika_peer *peer, uint8_t *salt)
{
    memcpy(peer->salt, salt, LAIKA_HANDSHAKE_SALT_LEN);
}

void laikaS_genSalt(struct sLaika_peer *peer)
{
    randombytes_buf(peer->salt, LAIKA_HANDSHAKE_SALT_LEN);
}

/* ===================================[[ Start/End Packets ]]=================================== */

void laikaS_emptyOutPacket(struct sLaika_peer *peer, LAIKAPKT_ID id)
{
    struct sLaika_socket *sock = &peer->sock;

    laikaS_writeByte(sock, id);

    /* add to pollList's out queue */
    laikaP_pushOutQueue(peer->pList, &peer->sock);
}

void laikaS_startOutPacket(struct sLaika_peer *peer, LAIKAPKT_ID id)
{
    struct sLaika_socket *sock = &peer->sock;

    if (peer->outStart != -1) /* sanity check */
        LAIKA_ERROR("unended OUT packet!\n");

    laikaS_writeByte(sock, id);

    peer->outStart = laikaM_countVector(sock->outBuf);
    if (peer->useSecure) { /* if we're encrypting this packet, append the nonce right after the
                              packet ID */
        uint8_t nonce[crypto_secretbox_NONCEBYTES];
        randombytes_buf(nonce, crypto_secretbox_NONCEBYTES);
        laikaS_write(sock, nonce, crypto_secretbox_NONCEBYTES);
    }
}

int laikaS_endOutPacket(struct sLaika_peer *peer)
{
    struct sLaika_socket *sock = &peer->sock;
    uint8_t *body;
    size_t sz;

    if (peer->useSecure) {
        /* make sure we have enough space */
        laikaM_growVector(uint8_t, sock->outBuf, crypto_secretbox_MACBYTES);

        /* packet body starts after the id & nonce */
        body = &sock->outBuf[peer->outStart + crypto_secretbox_NONCEBYTES];
        /* encrypt packet body in-place */
        if (crypto_secretbox_easy(body, body,
                                  (laikaM_countVector(sock->outBuf) - peer->outStart) -
                                      crypto_secretbox_NONCEBYTES,
                                  &sock->outBuf[peer->outStart], peer->outKey) != 0) {
            LAIKA_ERROR("Failed to encrypt packet!\n");
        }

        laikaM_countVector(sock->outBuf) += crypto_secretbox_MACBYTES;
    }

    /* add to pollList's out queue */
    laikaP_pushOutQueue(peer->pList, &peer->sock);

    /* return packet size and prepare for next outPacket */
    sz = laikaM_countVector(sock->outBuf) - peer->outStart;
    peer->outStart = -1;
    return sz;
}

void laikaS_startVarPacket(struct sLaika_peer *peer, LAIKAPKT_ID id)
{
    struct sLaika_socket *sock = &peer->sock;

    if (peer->outStart != -1) /* sanity check */
        LAIKA_ERROR("unended OUT packet!\n");

    laikaS_writeByte(sock, LAIKAPKT_VARPKT);
    laikaS_zeroWrite(sock,
                     sizeof(LAIKAPKT_SIZE)); /* allocate space for packet size to patch later */
    laikaS_startOutPacket(peer, id);
}

int laikaS_endVarPacket(struct sLaika_peer *peer)
{
    struct sLaika_socket *sock = &peer->sock;
    int patchIndx = peer->outStart -
                    (sizeof(LAIKAPKT_SIZE) + sizeof(LAIKAPKT_ID)); /* gets index of packet size */
    LAIKAPKT_SIZE sz = (LAIKAPKT_SIZE)laikaS_endOutPacket(peer);

    /* patch packet size */
    memcpy((void *)&sock->outBuf[patchIndx], (void *)&sz, sizeof(LAIKAPKT_SIZE));
    return sz;
}

void laikaS_startInPacket(struct sLaika_peer *peer, bool variadic)
{
    struct sLaika_socket *sock = &peer->sock;

    if (peer->inStart != -1) /* sanity check */
        LAIKA_ERROR("unended IN packet!\n");

    /* if we're encrypting/decrypting all packets, make sure to make the packetsize reflect this */
    if (peer->useSecure && !variadic && peer->pktSize != 0)
        peer->pktSize += crypto_secretbox_MACBYTES + crypto_secretbox_NONCEBYTES;

    peer->inStart = laikaM_countVector(sock->inBuf);
}

int laikaS_endInPacket(struct sLaika_peer *peer)
{
    struct sLaika_socket *sock = &peer->sock;
    uint8_t *body;
    size_t sz = laikaM_countVector(sock->inBuf) - peer->inStart;

    if (peer->useSecure && sz > crypto_secretbox_MACBYTES + crypto_secretbox_NONCEBYTES) {
        body = &sock->inBuf[peer->inStart + crypto_secretbox_NONCEBYTES];

        /* decrypt packet body in-place */
        if (crypto_secretbox_open_easy(body, body,
                                       (laikaM_countVector(sock->inBuf) - peer->inStart) -
                                           crypto_secretbox_NONCEBYTES,
                                       &sock->inBuf[peer->inStart], peer->inKey) != 0) {
            LAIKA_ERROR("Failed to decrypt packet!\n");
        }

        /* decrypted message is smaller now */
        laikaM_countVector(sock->inBuf) -= crypto_secretbox_MACBYTES;

        /* remove nonce */
        laikaM_rmvVector(sock->inBuf, peer->inStart, crypto_secretbox_NONCEBYTES);

        sz -= crypto_secretbox_MACBYTES + crypto_secretbox_NONCEBYTES;
    }

    peer->inStart = -1;
    return sz;
}

void laikaS_setSecure(struct sLaika_peer *peer, bool flag)
{
    peer->useSecure = flag;
}

/* ==================================[[ Handle Poll Events ]]=================================== */

bool laikaS_handlePeerIn(struct sLaika_socket *sock)
{
    struct sLaika_peer *peer = (struct sLaika_peer *)sock;
    int recvd;

    switch (peer->pktID) {
    case LAIKAPKT_MAXNONE:
        /* try grabbing pktID */
        if (laikaS_rawRecv(&peer->sock, sizeof(uint8_t), &recvd) != RAWSOCK_OK)
            return false;

        /* read packet ID */
        peer->pktID = laikaS_readByte(&peer->sock);
        LAIKA_DEBUG("%s\n", laikaD_getPacketName(peer->pktID));

        /* LAIKAPKT_VARPKT's body is unencrypted, and handled by this switch statement.
           LAIKAPKT_VARPKT is also likely not to be defined in our pktSizeTable. the LAIKAPKT_VARPKT
           case calls laikaS_startInPacket for itself, so skip all of this */
        if (peer->pktID == LAIKAPKT_VARPKT)
            goto _HandlePacketVariadic;

        /* sanity check pktID, pktID's handler & make sure it's not marked as variadic */
        if (peer->pktID >= LAIKAPKT_MAXNONE || peer->packetTbl[peer->pktID].handler == NULL ||
            peer->packetTbl[peer->pktID].variadic)
            LAIKA_ERROR("peer %p doesn't support packet id [%d]!\n", peer, peer->pktID);

        peer->pktSize = peer->packetTbl[peer->pktID].size;

        /* if peer->useSecure is true, body is encrypted */
        laikaS_startInPacket(peer, false);
        goto _HandlePacketBody;
    case LAIKAPKT_VARPKT:
    _HandlePacketVariadic:
        /* try grabbing pktID & size */
        if (laikaS_rawRecv(&peer->sock, sizeof(LAIKAPKT_ID) + sizeof(LAIKAPKT_SIZE), &recvd) !=
            RAWSOCK_OK)
            return false;

        /* not worth queuing & setting pollIn for 3 bytes. if the connection is that slow, it was
         * probably sent maliciously anyways */
        if (recvd != sizeof(LAIKAPKT_ID) + sizeof(LAIKAPKT_SIZE))
            LAIKA_ERROR("couldn't read whole LAIKAPKT_VARPKT\n");

        /* read packet size */
        laikaS_readInt(&peer->sock, (void *)&peer->pktSize, sizeof(LAIKAPKT_SIZE));

        if (peer->pktSize > LAIKA_MAX_PKTSIZE)
            LAIKA_ERROR("variable packet too large!\n");

        /* read pktID */
        peer->pktID = laikaS_readByte(&peer->sock);

        /* sanity check pktID, check valid handler & make sure it's marked as variadic */
        if (peer->pktID >= LAIKAPKT_MAXNONE || peer->packetTbl[peer->pktID].handler == NULL ||
            !peer->packetTbl[peer->pktID].variadic)
            LAIKA_ERROR("requested packet id [%d] is not variadic!\n", peer->pktID);

        /* sanity check minimum size */
        if (peer->pktSize <= peer->packetTbl[peer->pktID].size)
            LAIKA_ERROR("requested variable packet is too small!\n");

        /* if peer->useSecure is true, body is encrypted */
        laikaS_startInPacket(peer, true);
        goto _HandlePacketBody;
    default:
    _HandlePacketBody:
        /* try grabbing the rest of the packet */
        if (laikaS_rawRecv(&peer->sock, peer->pktSize - laikaM_countVector(peer->sock.inBuf),
                           &recvd) != RAWSOCK_OK)
            return false;

        /* have we received the full packet? */
        if (peer->pktSize == laikaM_countVector(peer->sock.inBuf)) {
            peer->pktSize = laikaS_endInPacket(peer);

            /* dispatch to packet handler */
            peer->packetTbl[peer->pktID].handler(peer, peer->pktSize, peer->uData);

            /* reset */
            laikaM_countVector(peer->sock.inBuf) = 0;
            peer->pktID = LAIKAPKT_MAXNONE;
        }

        break;
    }

    return laikaS_isAlive((&peer->sock));
}

bool laikaS_handlePeerOut(struct sLaika_socket *sock)
{
    struct sLaika_peer *peer = (struct sLaika_peer *)sock;
    int sent;

    if (laikaM_countVector(peer->sock.outBuf) == 0) /* sanity check */
        return true;

    switch (laikaS_rawSend(&peer->sock, laikaM_countVector(peer->sock.outBuf), &sent)) {
    case RAWSOCK_OK: /* we're ok! */
        /* if POLLOUT was set, unset it */
        laikaP_rmvPollOut(peer->pList, &peer->sock);

        return true;
    case RAWSOCK_POLL: /* we've been asked to set the POLLOUT flag */
        /* if POLLOUT wasn't set, set it so we'll be notified whenever the kernel has room :) */
        laikaP_addPollOut(peer->pList, &peer->sock);

        return true;
    default: /* panic! */
    case RAWSOCK_CLOSED:
    case RAWSOCK_ERROR:
        return false;
    }
}