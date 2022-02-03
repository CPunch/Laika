#ifndef LAIKA_SOCKET_H
#define LAIKA_SOCKET_H


/* socket/winsock headers */
#ifdef _WIN32
/* windows */
    #ifndef NOMINMAX
    #define NOMINMAX
    #endif
    #define _WINSOCK_DEPRECATED_NO_WARNINGS
    #include <winsock2.h>
    #include <windows.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib")

    typedef char buffer_t;
    #define PollFD WSAPOLLFD
    #define poll WSAPoll
    #define LN_ERRNO WSAGetLastError()
    #define LN_EWOULD WSAEWOULDBLOCK
    #define LN_MSG_NOSIGNAL 0
    #define SOCKETINVALID(x) (x == INVALID_SOCKET)
    #define SOCKETERROR(x) (x == SOCKET_ERROR)
#else
/* posix platform */
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <poll.h>
#ifdef __linux__
    #include <sys/epoll.h>
/* max events for epoll() */
    #define MAX_EPOLL_EVENTS 128
    #define LAIKA_USE_EPOLL 
#endif
    #include <unistd.h>
    #include <errno.h>

    typedef int SOCKET;
    typedef void buffer_t;
    #define PollFD struct pollfd
    #define LN_ERRNO errno
    #define LN_EWOULD EWOULDBLOCK
    #define LN_MSG_NOSIGNAL MSG_NOSIGNAL
    #define INVALID_SOCKET -1
    #define SOCKETINVALID(x) (x < 0)
    #define SOCKETERROR(x) (x == -1)
#endif
#include <fcntl.h>

#include "lrsa.h"

typedef enum {
    RAWSOCK_OK,
    RAWSOCK_ERROR,
    RAWSOCK_CLOSED,
    RAWSOCK_POLL
} RAWSOCKCODE;

struct sLaika_socket {
    uint8_t *outBuf; /* raw data to be sent() */
    uint8_t *inBuf; /* raw data we recv()'d */
    uint8_t inKey[crypto_kx_SESSIONKEYBYTES], outKey[crypto_kx_SESSIONKEYBYTES];
    SOCKET sock; /* raw socket fd */
    int outCount;
    int inCount;
    int outCap;
    int inCap;
    int outStart; /* index of pktID for out packet */
    int inStart; /* index of pktID for in packet */
    bool flipEndian;
    bool useSecure; /* if true, sock will transmit/receive encrypted data using inKey & outKey */
};

#define laikaS_isAlive(arg) (arg->sock != INVALID_SOCKET)

bool laikaS_isBigEndian(void);

void laikaS_init(void);
void laikaS_cleanUp(void);

void laikaS_initSocket(struct sLaika_socket *sock);
void laikaS_cleanSocket(struct sLaika_socket *sock);
void laikaS_kill(struct sLaika_socket *sock); /* kills a socket */
void laikaS_connect(struct sLaika_socket *sock, char *ip, char *port); /* connect to ip & port */
void laikaS_bind(struct sLaika_socket *sock, uint16_t port); /* bind sock to port */
void laikaS_acceptFrom(struct sLaika_socket *sock, struct sLaika_socket *from);
bool laikaS_setNonBlock(struct sLaika_socket *sock);

void laikaS_startOutPacket(struct sLaika_socket *sock, uint8_t id);
int laikaS_endOutPacket(struct sLaika_socket *sock);
void laikaS_startInPacket(struct sLaika_socket *sock);
int laikaS_endInPacket(struct sLaika_socket *sock);
void laikaS_setSecure(struct sLaika_socket *sock, bool flag);
void laikaS_read(struct sLaika_socket *sock, void *buf, size_t sz); /* reads from inBuf */
void laikaS_write(struct sLaika_socket *sock, void *buf, size_t sz); /* writes to outBuf */
void laikaS_writeKeyEncrypt(struct sLaika_socket *sock, void *buf, size_t sz, uint8_t *pub); /* encrypts & writes from buf using pub key */
void laikaS_readKeyDecrypt(struct sLaika_socket *sock, void *buf, size_t sz, uint8_t *pub, uint8_t *priv); /* decrypts & reads to buf using pub & priv key*/
void laikaS_writeByte(struct sLaika_socket *sock, uint8_t data);
uint8_t laikaS_readByte(struct sLaika_socket *sock);
void laikaS_readInt(struct sLaika_socket *sock, void *buf, size_t sz); /* reads INT, respecting endianness */
void laikaS_writeInt(struct sLaika_socket *sock, void *buf, size_t sz); /* writes INT, respecting endianness */

RAWSOCKCODE laikaS_rawRecv(struct sLaika_socket *sock, size_t sz, int *processed);
RAWSOCKCODE laikaS_rawSend(struct sLaika_socket *sock, size_t sz, int *processed);

#endif