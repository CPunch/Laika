#ifndef LAIKA_PEER_H
#define LAIKA_PEER_H

#include "laika.h"
#include "lsocket.h"
#include "lpacket.h"
#include "lpolllist.h"

struct sLaika_peer {
    struct sLaika_socket sock; /* DO NOT MOVE THIS. this member HAS TO BE FIRST so that typecasting sLaika_peer* to sLaika_sock* works as intended */
    struct sLaika_pollList *pList; /* pollList we're active in */
    void (*pktHandler)(struct sLaika_peer *peer, LAIKAPKT_ID id, void *uData);
    void *uData; /* data to be passed to pktHandler */
    size_t *pktSizeTable; /* const table to pull pkt size data from */
    size_t pktSize; /* current pkt size */
    LAIKAPKT_ID pktID; /* current pkt ID */
    bool setPollOut; /* is EPOLLOUT/POLLOUT is set on sock's pollfd ? */
};

struct sLaika_peer *laikaS_newPeer(void (*pktHandler)(struct sLaika_peer *peer, LAIKAPKT_ID id, void *uData), size_t *pktSizeTable, struct sLaika_pollList *pList, void *uData);
void laikaS_freePeer(struct sLaika_peer *peer);

bool laikaS_handlePeerIn(struct sLaika_peer *peer);
bool laikaS_handlePeerOut(struct sLaika_peer *peer);

#endif