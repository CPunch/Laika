#ifndef LAIKA_PEER_H
#define LAIKA_PEER_H

#include "laika.h"
#include "lsocket.h"
#include "lpacket.h"
#include "lpolllist.h"

typedef enum {
    PEER_BOT,
    PEER_CNC, /* cnc 2 cnc communication */
    PEER_AUTH /* authorized peers can send commands to cnc */
} PEERTYPE;

struct sLaika_peer {
    struct sLaika_socket sock; /* DO NOT MOVE THIS. this member HAS TO BE FIRST so that typecasting sLaika_peer* to sLaika_sock* works as intended */
    struct sLaika_pollList *pList; /* pollList we're active in */
    void (*pktHandler)(struct sLaika_peer *peer, uint8_t id, void *uData);
    void *uData; /* data to be passed to pktHandler */
    LAIKAPKT_SIZE *pktSizeTable; /* const table to pull pkt size data from */
    LAIKAPKT_SIZE pktSize; /* current pkt size */
    LAIKAPKT_ID pktID; /* current pkt ID */
    PEERTYPE type;
    bool setPollOut; /* is EPOLLOUT/POLLOUT is set on sock's pollfd ? */
};

struct sLaika_peer *laikaS_newPeer(void (*pktHandler)(struct sLaika_peer *peer, LAIKAPKT_ID id, void *uData), LAIKAPKT_SIZE *pktSizeTable, struct sLaika_pollList *pList, void *uData);
void laikaS_freePeer(struct sLaika_peer *peer);

bool laikaS_handlePeerIn(struct sLaika_peer *peer);
bool laikaS_handlePeerOut(struct sLaika_peer *peer);

#endif