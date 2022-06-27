#ifndef LAIKA_POLLLIST_H
#define LAIKA_POLLLIST_H

#include "hashmap.h"
#include "laika.h"
#include "lsocket.h"

#include <stdbool.h>

/* number of pollFDs or epollFDs we expect to start with */
#define POLLSTARTCAP 8

struct sLaika_pollEvent
{
    struct sLaika_socket *sock;
    bool pollIn;
    bool pollOut;
};

struct sLaika_pollList
{
    struct hashmap *sockets;
    struct sLaika_socket **outQueue; /* holds sockets which have data needed to be sent */
    struct sLaika_pollEvent *revents;
#ifdef LAIKA_USE_EPOLL
    /* epoll */
    struct epoll_event ev, ep_events[MAX_EPOLL_EVENTS];
    SOCKET epollfd;
#else
    /* raw poll descriptor */
    PollFD *fds;
    int fdCapacity;
    int fdCount;
#endif
    int reventCap;
    int reventCount;
    int outCap;
    int outCount;
};

void laikaP_initPList(struct sLaika_pollList *pList);
void laikaP_cleanPList(struct sLaika_pollList *pList); /* free's all members */
void laikaP_addSock(struct sLaika_pollList *pList, struct sLaika_socket *sock);
void laikaP_rmvSock(struct sLaika_pollList *pList, struct sLaika_socket *sock);
void laikaP_addPollOut(struct sLaika_pollList *pList, struct sLaika_socket *sock);
void laikaP_rmvPollOut(struct sLaika_pollList *pList, struct sLaika_socket *sock);
void laikaP_pushOutQueue(struct sLaika_pollList *pList, struct sLaika_socket *sock);
void laikaP_resetOutQueue(struct sLaika_pollList *pList);
void laikaP_flushOutQueue(struct sLaika_pollList *pList);

struct sLaika_pollEvent *laikaP_poll(struct sLaika_pollList *pList, int timeout, int *nevents);
bool laikaP_handleEvent(struct sLaika_pollEvent *evnt);

#endif