#ifndef LAIKA_PEER_H
#define LAIKA_PEER_H

#include "laika.h"
#include "lsocket.h"
#include "lpacket.h"
#include "lpolllist.h"

struct sLaika_peer {
    struct sLaika_socket sock;
    struct sLaika_pollList *pList; /* pollList we're active in */
    void (*pktHandler)(struct sLaika_peer *peer, LAIKAPKT_ID id);
    size_t pktSize; /* current pkt size */
    LAIKAPKT_ID pktID; /* current pkt ID */
    size_t *pktSizeTable; /* const table to pull pkt size data from */
    bool setPollOut; /* if EPOLLOUT/POLLOUT is set on sock's pollfd */
};

struct sLaika_peer *laikaS_newPeer(void (*pktHandler)(struct sLaika_peer *peer, LAIKAPKT_ID id), struct sLaika_pollList *pList, size_t *pktSizeTable);
void laikaS_freePeer(struct sLaika_peer *peer);

bool laikaS_handlePeerIn(struct sLaika_peer *peer);
bool laikaS_handlePeerOut(struct sLaika_peer *peer);

#endif