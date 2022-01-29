#include "lerror.h"
#include "lmem.h"
#include "lpolllist.h"

typedef struct sLaika_hashMapElem {
    SOCKET fd;
    struct sLaika_socket *sock;
} tLaika_hashMapElem;

int elem_compare(const void *a, const void *b, void *udata) {
    const tLaika_hashMapElem *ua = a;
    const tLaika_hashMapElem *ub = b;

    return ua->fd != ub->fd; 
}

uint64_t elem_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const tLaika_hashMapElem *u = item;
    return (uint64_t)(u->fd);
}

void laikaP_initPList(struct sLaika_pollList *pList) {
    laikaS_init();

    /* setup hashmap */
    pList->sockets = hashmap_new(sizeof(tLaika_hashMapElem), POLLSTARTCAP, 0, 0, elem_hash, elem_compare, NULL, NULL);
    pList->revents = NULL; /* laikaP_pollList() will allocate the buffer */
    pList->reventCapacity = POLLSTARTCAP/GROW_FACTOR;
    pList->reventCount = 0;

#ifdef LAIKA_USE_EPOLL
    /* setup our epoll */
    memset(&pList->ev, 0, sizeof(struct epoll_event));
    if ((pList->epollfd = epoll_create(POLLSTARTCAP)) == -1)
        LAIKA_ERROR("epoll_create() failed!\n");

#else
    pList->fds = NULL; /* laikaP_addSock will allocate the buffer */
    pList->fdCapacity = POLLSTARTCAP/GROW_FACTOR; /* div by GROW_FACTOR since laikaM_growarray multiplies by GROW_FACTOR */
    pList->fdCount = 0;
#endif
}

void laikaP_cleanPList(struct sLaika_pollList *pList) {
    laikaM_free(pList->revents);
    hashmap_free(pList->sockets);

#ifdef LAIKA_USE_EPOLL
    close(pList->epollfd);
#else
    laikaM_free(pList->fds);
#endif

    laikaS_cleanUp();
}

void laikaP_addSock(struct sLaika_pollList *pList, struct sLaika_socket *sock) {
    /* add socket to hashmap */
    hashmap_set(pList->sockets, &(tLaika_hashMapElem){.fd = sock->sock, .sock = sock});

#ifdef LAIKA_USE_EPOLL
    pList->ev.events = EPOLLIN;
    pList->ev.data.ptr = (void*)sock;

    if (epoll_ctl(pList->epollfd, EPOLL_CTL_ADD, sock->sock, &pList->ev) == -1)
        LAIKA_ERROR("epoll_ctl [ADD] failed\n");

#else
    /* allocate space in array & add PollFD */
    laikaM_growarray(PollFD, pList->fds, 1, pList->fdCount, pList->fdCapacity);
    pList->fds[pList->fdCount++] = (PollFD){sock->sock, POLLIN};
#endif
}

void laikaP_rmvSock(struct sLaika_pollList *pList, struct sLaika_socket *sock) {
    /* remove socket from hashmap */
    hashmap_delete(pList->sockets, &(tLaika_hashMapElem){.fd = sock->sock, .sock = sock});

#ifdef LAIKA_USE_EPOLL
    /* epoll_event* isn't needed with EPOLL_CTL_DEL, however we still need to pass a NON-NULL pointer. [see: https://man7.org/linux/man-pages/man2/epoll_ctl.2.html#BUGS] */
    if (epoll_ctl(pList->epollfd, EPOLL_CTL_DEL, sock->sock, &pList->ev) == -1) {
        /* non-fatal error, socket probably just didn't exist, so ignore it. */
        LAIKA_WARN("epoll_ctl [DEL] failed\n");
    }
#else
    int i;

    /* search fds for socket, remove it and shrink array */
    for (i = 0; i < pList->fdCount; i++) {
        if (pList->fds[i].fd == sock->sock) {
            /* remove from array */
            laikaM_rmvarray(PollFD, pList->fds, pList->fdCount, i, 1);
            break;
        }
    }
#endif
}

void laikaP_addPollOut(struct sLaika_pollList *pList, struct sLaika_socket *sock) {
#ifdef LAIKA_USE_EPOLL
    pList->ev.events = EPOLLIN | EPOLLOUT;
    pList->ev.data.ptr = (void*)sock;
    if (epoll_ctl(pList->epollfd, EPOLL_CTL_MOD, sock->sock, &pList->ev) == -1) {
        /* non-fatal error, socket probably just didn't exist, so ignore it. */
        LAIKA_WARN("epoll_ctl [MOD] failed\n");
    }
#else
    int i;

    /* search fds for socket, add POLLOUT flag */
    for (i = 0; i < pList->fdCount; i++) {
        if (pList->fds[i].fd == sock->sock) {
            pList->fds[i].events = POLLIN | POLLOUT;
            break;
        }
    }
#endif
}

void laikaP_rmvPollOut(struct sLaika_pollList *pList, struct sLaika_socket *sock) {
#ifdef LAIKA_USE_EPOLL
    pList->ev.events = EPOLLIN;
    pList->ev.data.ptr = (void*)sock;
    if (epoll_ctl(pList->epollfd, EPOLL_CTL_MOD, sock->sock, &pList->ev) == -1) {
        /* non-fatal error, socket probably just didn't exist, so ignore it. */
        LAIKA_WARN("epoll_ctl [MOD] failed\n");
    }
#else
    int i;

    /* search fds for socket, remove POLLOUT flag */
    for (i = 0; i < pList->fdCount; i++) {
        if (pList->fds[i].fd == sock->sock) {
            pList->fds[i].events = POLLIN;
            break;
        }
    }
#endif
}

struct sLaika_pollEvent *laikaP_poll(struct sLaika_pollList *pList, int timeout, int *_nevents) {
    int nEvents, i;

    pList->reventCount = 0; /* reset revent array */
#ifdef LAIKA_USE_EPOLL
/* fastpath: we store the sLaika_socket* pointer directly in the epoll_data_t, saving us a lookup into our socket hashmap
      not to mention the various improvements epoll() has over poll() :D
*/
    nEvents = epoll_wait(pList->epollfd, pList->ep_events, MAX_EPOLL_EVENTS, timeout);

    if (SOCKETERROR(nEvents))
        LAIKA_ERROR("epoll_wait() failed!\n");

    for (i = 0; i < nEvents; i++) {
        /* add event to revent array */
        laikaM_growarray(struct sLaika_pollEvent, pList->revents, 1, pList->reventCount, pList->reventCapacity);
        pList->revents[pList->reventCount++] = (struct sLaika_pollEvent){
            .sock = pList->ep_events[i].data.ptr,
            .pollIn = pList->ep_events[i].events & EPOLLIN,
            .pollOut = pList->ep_events[i].events & EPOLLOUT
        };
    }
#else
    nEvents = poll(pList->fds, pList->fdCount, timeout); /* poll returns -1 for error, or the number of events */

    if (SOCKETERROR(nEvents))
        LAIKA_ERROR("poll() failed!\n");

    /* walk through the returned poll fds, if they have an event, add it to our revents array */
    for (i = 0; i < pList->fdCount && nEvents > 0; i++) {
        PollFD pfd = pList->fds[i];
        if (pList->fds[i].revents != 0) {
            /* grab socket from hashmap */
            struct sLaika_socket *sock = hashmap_get(pList->sockets, &(tLaika_hashMapElem){.fd = (SOCKET)pfd.fd});

            /* insert event into revents array */
            laikaM_growarray(struct sLaika_pollEvent, pList->revents, 1, pList->reventCount, pList->reventCapacity);
            pList->revents[pList->reventCount++] = (struct sLaika_pollEvent){
                .sock = sock,
                .pollIn = pfd.revents & POLLIN,
                .pollOut = pfd.revents & POLLOUT
            };

            nEvents--;
        }
    }
#endif

    /* return revents array */
    *_nevents = i;
    return pList->revents;
}
