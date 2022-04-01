#include "lmem.h"
#include "lpeer.h"
#include "lpacket.h"
#include "lpolllist.h"
#include "ltunnel.h"

struct sLaika_tunnel *laikaT_newTunnel(struct sLaika_peer *peer, uint16_t port) {
    struct sLaika_tunnel *tunnel = (struct sLaika_tunnel*)laikaM_malloc(sizeof(struct sLaika_tunnel));

    tunnel->port = port;
    tunnel->peer = peer;
    tunnel->connectionHead = NULL;
}

void laikaT_freeTunnel(struct sLaika_tunnel *tunnel) {
    struct sLaika_tunnelConnection *con = tunnel->connectionHead, *last;
    struct sLaika_socket *sock = &tunnel->peer->sock;
    
    /* free & close connections */
    while (con != NULL) {
        last = con;
        con = con->next;

        /* free connection */
        laikaT_freeConnection(last);
    }

    /* tell peer tunnel is closed */
    laikaS_startOutPacket(tunnel->peer, LAIKAPKT_TUNNEL_CLOSE);
    laikaS_writeInt(sock, &tunnel->port, sizeof(uint16_t));
    laikaS_endOutPacket(tunnel->peer);

    /* finally, free the tunnel */
    laikaM_free(tunnel);
}

bool laikaT_handlePeerIn(struct sLaika_socket *sock) {
    struct sLaika_tunnelConnection *con = (struct sLaika_tunnelConnection*)sock;
    struct sLaika_tunnel *tunnel = con->tunnel;
    struct sLaika_peer *peer = tunnel->peer;
    int recvd;

    /* read data */
    switch (laikaS_rawRecv(&con->sock, LAIKA_MAX_PKTSIZE - (sizeof(uint16_t) + sizeof(uint16_t)), &recvd)) { /* - 4 since we need room in the packet for the id & port */
        case RAWSOCK_OK: /* we're ok! forward data to peer */
            laikaS_startVarPacket(peer, LAIKAPKT_TUNNEL_CONNECTION_DATA);
            laikaS_writeInt(&peer->sock, &tunnel->port, sizeof(uint16_t));
            laikaS_writeInt(&peer->sock, &con->id, sizeof(uint16_t));

            /* write data we just read, from sock's inBuf to sock's out */
            laikaS_write(&peer->sock, (void*)peer->sock.inBuf, recvd);
            laikaS_consumeRead(&peer->sock, recvd);

            /* end variadic packet */
            laikaS_endVarPacket(peer);
        default: /* panic! */
        case RAWSOCK_CLOSED:
        case RAWSOCK_ERROR:
            return false;
    }
}

bool laikaT_handlePeerOut(struct sLaika_socket *sock) {
    struct sLaika_tunnelConnection *con = (struct sLaika_tunnelConnection*)sock;
    struct sLaika_peer *peer = con->tunnel->peer;
    int sent;

    if (peer->sock.outCount == 0) /* sanity check */
        return true;

    switch (laikaS_rawSend(&con->sock, con->sock.outCount, &sent)) {
        case RAWSOCK_OK: /* we're ok! */
            /* if POLLOUT was set, unset it */
            laikaP_rmvPollOut(peer->pList, &con->sock);

            return true;
        case RAWSOCK_POLL: /* we've been asked to set the POLLOUT flag */
            /* if POLLOUT wasn't set, set it so we'll be notified whenever the kernel has room :) */
            laikaP_addPollOut(peer->pList, &con->sock);

            return true;
        default: /* panic! */
        case RAWSOCK_CLOSED:
        case RAWSOCK_ERROR:
            return false;
    }

}

void laikaT_onPollFail(struct sLaika_socket *sock, void *uData) {
    struct sLaika_tunnelConnection *con = (struct sLaika_tunnelConnection*)sock;

    /* kill connection on failure */
    laikaT_freeConnection(con);
}

struct sLaika_tunnelConnection *laikaT_newConnection(struct sLaika_tunnel *tunnel, uint16_t id) {
    struct sLaika_tunnelConnection *con = (struct sLaika_tunnelConnection*)laikaM_malloc(sizeof(struct sLaika_tunnelConnection)), *last = NULL;

    /* we handle the socket events */
    laikaS_initSocket(&con->sock,
        laikaT_handlePeerIn,
        laikaT_handlePeerOut,
        laikaT_onPollFail,
        NULL
    );

    laikaS_setNonBlock(&con->sock);

    con->tunnel = tunnel;
    con->next = NULL;
    con->id = id;

    /* insert into connection list */
    if (tunnel->connectionHead == NULL) {
        tunnel->connectionHead = con;
        return con;
    } else {
        last = tunnel->connectionHead;
        tunnel->connectionHead = con;
        con->next = last;
    }
}

void laikaT_freeConnection(struct sLaika_tunnelConnection *connection) {
    struct sLaika_tunnel *tunnel = connection->tunnel;
    struct sLaika_tunnelConnection *curr, *last = NULL;

    curr = tunnel->connectionHead;
    /* while we haven't reached the end of the list & we haven't found our connection */
    while (curr && curr != connection) {
        last = curr;
        curr = curr->next;
    }

    if (last) {
        /* unlink from list */
        last->next = connection->next;
    } else { /* connectionHead was NULL, or connection *was* the connectionHead. */
        tunnel->connectionHead = connection->next;
    }

    /* tell peer connection is removed */
    laikaS_startOutPacket(tunnel->peer, LAIKAPKT_TUNNEL_CONNECTION_RMV);
    laikaS_writeInt(&tunnel->peer->sock, &tunnel->port, sizeof(uint16_t));
    laikaS_writeInt(&tunnel->peer->sock, &connection->id, sizeof(uint16_t));
    laikaS_endOutPacket(tunnel->peer);

    /* finally, free our connection */
    laikaS_kill(&connection->sock);
}

void laikaT_forwardData(struct sLaika_tunnelConnection *connection, struct sLaika_pollList *pList, void *data, size_t sz) {
    struct sLaika_tunnel *tunnel = connection->tunnel;

    /* write data to socket, push to pollList's out queue */
    laikaS_write(&connection->sock, data, sz);
    laikaP_pushOutQueue(pList, &connection->sock);
}

struct sLaika_tunnelConnection *laikaT_getConnection(struct sLaika_tunnel *tunnel, uint16_t id) {
    struct sLaika_tunnelConnection *curr = tunnel->connectionHead;

    /* search for the id in the linked list. curr->next is guaranteed to be NULL at the end of the list */
    while (curr && curr->id != id)
        curr = curr->next;

    /* returns NULL if not found, or the found connection :) */
    return curr;
}