#ifndef SHELLTUNNEL_H
#define SHELLTUNNEL_H

#include <inttypes.h>

#include "lmem.h"
#include "lsocket.h"
#include "lpeer.h"
#include "lpolllist.h"

struct sLaika_tunnel;
struct sLaika_tunnelConnection {
    struct sLaika_socket sock;
    struct sLaika_tunnel *tunnel;
    struct sLaika_tunnelConnection *next;
    uint16_t id;
};

struct sLaika_tunnel {
    struct sLaika_tunnelConnection *connectionHead;
    struct sLaika_peer *peer;
    uint16_t port;
};

struct sLaika_tunnel *laikaT_newTunnel(struct sLaika_peer *peer, uint16_t port);
void laikaT_freeTunnel(struct sLaika_tunnel *tunnel);

struct sLaika_tunnelConnection *laikaT_newConnection(struct sLaika_tunnel *tunnel, uint16_t id);
void laikaT_freeConnection(struct sLaika_tunnelConnection *connection);

void laikaT_forwardData(struct sLaika_tunnelConnection *connection, struct sLaika_pollList *pList, void *data, size_t sz);
struct sLaika_tunnelConnection *laikaT_getConnection(struct sLaika_tunnel *tunnel, uint16_t id);

#endif