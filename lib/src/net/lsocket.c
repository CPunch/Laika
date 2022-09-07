#include "net/lsocket.h"

#include "core/lerror.h"
#include "core/lmem.h"
#include "core/lsodium.h"
#include "net/lpacket.h"
#include "net/lpolllist.h"

static int _LNSetup = 0;

void laikaS_init(void)
{
    if (_LNSetup++ > 0)
        return; /* WSA is already setup! */

#ifdef _WIN32
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0)
        LAIKA_ERROR("WSAStartup failed!\n");
#endif
}

void laikaS_cleanUp(void)
{
    if (--_LNSetup > 0)
        return; /* WSA still needs to be up, a socket is still running */

#ifdef _WIN32
    WSACleanup();
#endif
}

void laikaS_initSocket(struct sLaika_socket *sock, pollEvent onPollIn, pollEvent onPollOut,
                       pollFailEvent onPollFail, void *uData)
{
    sock->sock = INVALID_SOCKET;
    sock->onPollFail = onPollFail;
    sock->onPollIn = onPollIn;
    sock->onPollOut = onPollOut;
    sock->uData = uData;
    laikaM_initVector(sock->inBuf, 8);
    laikaM_initVector(sock->outBuf, 8);
    sock->flipEndian = false;
    sock->setPollOut = false;

    laikaS_init();
}

void laikaS_cleanSocket(struct sLaika_socket *sock)
{
    /* free in & out arrays */
    laikaM_free(sock->inBuf);
    laikaM_free(sock->outBuf);

    /* kill socket & cleanup WSA */
    laikaS_kill(sock);
    laikaS_cleanUp();
}

void laikaS_kill(struct sLaika_socket *sock)
{
    if (!laikaS_isAlive(sock)) /* sanity check */
        return;

#ifdef _WIN32
    shutdown(sock->sock, SD_BOTH);
    closesocket(sock->sock);
#else
    shutdown(sock->sock, SHUT_RDWR);
    close(sock->sock);
#endif

    sock->sock = INVALID_SOCKET;
}

void laikaS_connect(struct sLaika_socket *sock, char *ip, char *port)
{
    struct addrinfo res, *result, *curr;

    if (!SOCKETINVALID(sock->sock))
        LAIKA_ERROR("socket already setup!\n");

    /* zero out our address info and setup the type */
    memset(&res, 0, sizeof(struct addrinfo));
    res.ai_family = AF_UNSPEC;
    res.ai_socktype = SOCK_STREAM;

    /* grab the address info */
    if (getaddrinfo(ip, port, &res, &result) != 0)
        LAIKA_ERROR("getaddrinfo() failed!\n");

    /* getaddrinfo returns a list of possible addresses, step through them and try them until we
     * find a valid address */
    for (curr = result; curr != NULL; curr = curr->ai_next) {
        sock->sock = socket(curr->ai_family, curr->ai_socktype, curr->ai_protocol);

        /* if it failed, try the next sock */
        if (SOCKETINVALID(sock->sock))
            continue;

        /* if it's not an invalid socket, break and exit the loop, we found a working addr! */
        if (!SOCKETINVALID(connect(sock->sock, curr->ai_addr, curr->ai_addrlen)))
            break;

        laikaS_kill(sock);
    }
    freeaddrinfo(result);

    /* if we reached the end of the linked list, we failed looking up the addr */
    if (curr == NULL)
        LAIKA_ERROR("couldn't connect a valid address handle to socket!\n");
}

void laikaS_bind(struct sLaika_socket *sock, uint16_t port)
{
    socklen_t addressSize;
    struct sockaddr_in6 address;
    int opt = 1;

    if (!SOCKETINVALID(sock->sock))
        LAIKA_ERROR("socket already setup!\n");

    /* open our socket */
    sock->sock = socket(AF_INET6, SOCK_STREAM, 0);
    if (SOCKETINVALID(sock->sock))
        LAIKA_ERROR("socket() failed!\n");

        /* allow reuse of local address */
#ifdef _WIN32
    if (setsockopt(sock->sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(int)) != 0)
#else
    if (setsockopt(sock->sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) != 0)
#endif
        LAIKA_ERROR("setsockopt() failed!\n");

    address.sin6_family = AF_INET6;
    address.sin6_addr = in6addr_any;
    address.sin6_port = htons(port);

    addressSize = sizeof(address);

    /* bind to the port */
    if (SOCKETERROR(bind(sock->sock, (struct sockaddr *)&address, addressSize)))
        LAIKA_ERROR("bind() failed!\n");

    if (SOCKETERROR(listen(sock->sock, SOMAXCONN)))
        LAIKA_ERROR("listen() failed!\n");
}

void laikaS_acceptFrom(struct sLaika_socket *sock, struct sLaika_socket *from, char *ip)
{
    struct sockaddr_in6 address;
    socklen_t addressSize = sizeof(address);

    sock->sock = accept(from->sock, (struct sockaddr *)&address, &addressSize);
    if (SOCKETINVALID(sock->sock))
        LAIKA_ERROR("accept() failed!\n");

    /* read ip */
    if (ip) {
        if (inet_ntop(AF_INET6, &address.sin6_addr, ip, LAIKA_IPSTR_LEN) == NULL)
            LAIKA_ERROR("inet_ntop() failed!\n");
    }
}

bool laikaS_setNonBlock(struct sLaika_socket *sock)
{
#ifdef _WIN32
    unsigned long mode = 1;
    if (ioctlsocket(sock->sock, FIONBIO, &mode) != 0) {
#else
    if (fcntl(sock->sock, F_SETFL, (fcntl(sock->sock, F_GETFL, 0) | O_NONBLOCK)) != 0) {
#endif
        LAIKA_WARN("fcntl failed on new connection\n");
        laikaS_kill(sock);
        return false;
    }

    return true;
}

void laikaS_consumeRead(struct sLaika_socket *sock, size_t sz)
{
    laikaM_rmvVector(sock->inBuf, 0, sz);
}

void laikaS_zeroWrite(struct sLaika_socket *sock, size_t sz)
{
    laikaM_growVector(uint8_t, sock->outBuf, sz);

    /* set NULL bytes */
    memset(&sock->outBuf[laikaM_countVector(sock->outBuf)], 0, sz);
    laikaM_countVector(sock->outBuf) += sz;
}

void laikaS_read(struct sLaika_socket *sock, void *buf, size_t sz)
{
    memcpy(buf, sock->inBuf, sz);
    laikaM_rmvVector(sock->inBuf, 0, sz);
}

void laikaS_write(struct sLaika_socket *sock, void *buf, size_t sz)
{
    /* make sure we have enough space to copy the buffer */
    laikaM_growVector(uint8_t, sock->outBuf, sz);

    /* copy the buffer, then increment outCount */
    memcpy(&sock->outBuf[laikaM_countVector(sock->outBuf)], buf, sz);
    laikaM_countVector(sock->outBuf) += sz;
}

void laikaS_writeKeyEncrypt(struct sLaika_socket *sock, void *buf, size_t sz, uint8_t *pub)
{
    /* make sure we have enough space to encrypt the buffer */
    laikaM_growVector(uint8_t, sock->outBuf, LAIKAENC_SIZE(sz));

    /* encrypt the buffer into outBuf */
    if (crypto_box_seal(&sock->outBuf[laikaM_countVector(sock->outBuf)], buf, sz, pub) != 0)
        LAIKA_ERROR("Failed to encrypt!\n");

    laikaM_countVector(sock->outBuf) += LAIKAENC_SIZE(sz);
}

void laikaS_readKeyDecrypt(struct sLaika_socket *sock, void *buf, size_t sz, uint8_t *pub,
                           uint8_t *priv)
{
    /* decrypt into buf */
    if (crypto_box_seal_open(buf, sock->inBuf, LAIKAENC_SIZE(sz), pub, priv) != 0)
        LAIKA_ERROR("Failed to decrypt!\n");

    laikaM_rmvVector(sock->inBuf, 0, LAIKAENC_SIZE(sz));
}

void laikaS_writeByte(struct sLaika_socket *sock, uint8_t data)
{
    laikaM_growVector(uint8_t, sock->outBuf, 1);
    sock->outBuf[laikaM_countVector(sock->outBuf)++] = data;
}

uint8_t laikaS_readByte(struct sLaika_socket *sock)
{
    uint8_t tmp = *sock->inBuf;

    /* consume 1 byte */
    laikaM_rmvVector(sock->inBuf, 0, 1);
    return tmp;
}

void laikaS_writeu16(struct sLaika_socket *sock, uint16_t i)
{
    uint16_t tmp = i; /* copy int to buffer (which we can reverse if need-be) */

    if (sock->flipEndian)
        laikaM_reverse((uint8_t *)&tmp, sizeof(tmp));

    laikaS_write(sock, (void *)&tmp, sizeof(tmp));
}

uint16_t laikaS_readu16(struct sLaika_socket *sock)
{
    uint16_t tmp;
    laikaS_read(sock, (void *)&tmp, sizeof(tmp));

    if (sock->flipEndian)
        laikaM_reverse((uint8_t *)&tmp, sizeof(tmp));

    return tmp;
}

void laikaS_writeu32(struct sLaika_socket *sock, uint32_t i)
{
    uint32_t tmp = i; /* copy int to buffer (which we can reverse if need-be) */

    if (sock->flipEndian)
        laikaM_reverse((uint8_t *)&tmp, sizeof(tmp));

    laikaS_write(sock, (void *)&tmp, sizeof(tmp));
}

uint32_t laikaS_readu32(struct sLaika_socket *sock)
{
    uint32_t tmp;
    laikaS_read(sock, (void *)&tmp, sizeof(tmp));

    if (sock->flipEndian)
        laikaM_reverse((uint8_t *)&tmp, sizeof(tmp));

    return tmp;
}

RAWSOCKCODE laikaS_rawRecv(struct sLaika_socket *sock, size_t sz, int *processed)
{
    RAWSOCKCODE errCode = RAWSOCK_OK;
    int i, rcvd, start = laikaM_countVector(sock->inBuf);

    /* sanity check */
    if (sz == 0)
        return RAWSOCK_OK;

    /* make sure we have enough space to recv */
    laikaM_growVector(uint8_t, sock->inBuf, sz);
    rcvd = recv(sock->sock, (buffer_t *)&sock->inBuf[laikaM_countVector(sock->inBuf)], sz,
                LN_MSG_NOSIGNAL);

    if (rcvd == 0) {
        errCode = RAWSOCK_CLOSED;
    } else if (SOCKETERROR(rcvd) &&
               LN_ERRNO != LN_EWOULD
#ifndef _WIN32
               /* if it's a posix system, also make sure its not a EAGAIN result (which is a
                  recoverable error, there's just nothing to read lol) */
               && LN_ERRNO != EAGAIN
#endif
    ) {
        /* if the socket closed or an error occurred, return the error result */
        errCode = RAWSOCK_ERROR;
    } else if (rcvd > 0) {
#if 0
        /* for debugging */
        printf("---recv'd %d bytes---\n", rcvd);
        for (i = 1; i <= rcvd; i++) {
            printf("%.2x ", sock->inBuf[sock->inCount + (i-1)]);
            if (i % 16 == 0) {
                printf("\n");
            } else if (i % 8 == 0) {
                printf("\t");
            }
        }
        printf("\n");
#endif

        /* recv() worked, add rcvd to inCount */
        laikaM_countVector(sock->inBuf) += rcvd;
    }
    *processed = rcvd;
    return errCode;
}

RAWSOCKCODE laikaS_rawSend(struct sLaika_socket *sock, size_t sz, int *processed)
{
    RAWSOCKCODE errCode = RAWSOCK_OK;
    int sent, i, sentBytes = 0;

    /* write bytes to the socket until an error occurs or we finish sending */
    do {
        sent = send(sock->sock, (buffer_t *)(&sock->outBuf[sentBytes]), sz - sentBytes,
                    LN_MSG_NOSIGNAL);

        /* check for error result */
        if (sent == 0) { /* connection closed gracefully */
            errCode = RAWSOCK_CLOSED;
            goto _rawWriteExit;
        } else if (SOCKETERROR(sent)) { /* socket error? */
            if (LN_ERRNO != LN_EWOULD
#ifndef _WIN32
                /* posix also has some platforms which define EAGAIN as a different value than
                   EWOULD, might as well support it. */
                && LN_ERRNO != EAGAIN
#endif
            ) { /* socket error! */
                errCode = RAWSOCK_ERROR;
                goto _rawWriteExit;
            }

            /*
                it was a result of EWOULD or EAGAIN, kernel socket send buffer is full,
                tell the caller we need to set our poll event POLLOUT
            */
            errCode = RAWSOCK_POLL;
            goto _rawWriteExit;
        }
    } while ((sentBytes += sent) < sz);

_rawWriteExit:
#if 0
    /* for debugging */
    printf("---sent %d bytes---\n", sent);
    for (i = 1; i <= sentBytes; i++) {
        printf("%.2x ", sock->outBuf[i-1]);
        if (i % 16 == 0) {
            printf("\n");
        } else if (i % 8 == 0) {
            printf("\t");
        }
    }
    printf("\n");
#endif

    /* trim sent data from outBuf */
    laikaM_rmvVector(sock->outBuf, 0, sentBytes);

    *processed = sentBytes;
    return errCode;
}