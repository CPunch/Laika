#ifndef LAIKA_PEER_H
#define LAIKA_PEER_H

#include "core/lsodium.h"
#include "laika.h"
#include "net/lpacket.h"
#include "net/lpolllist.h"
#include "net/lsocket.h"

typedef enum
{
    PEER_PEER, /* unlogged-in peer */
    PEER_BOT,
    PEER_CNC, /* cnc 2 cnc communication (unused) */
    PEER_AUTH /* authorized peers can send commands to cnc */
} PEERTYPE;

typedef enum
{
    OS_UNKNWN,
    OS_WIN,
    OS_LIN
} OSTYPE;

#ifdef _WIN32
#    define LAIKA_OSTYPE OS_WIN
#else
#    ifdef __linux__
#        define LAIKA_OSTYPE OS_LIN
#    else
#        define LAIKA_OSTYPE OS_UNKNWN
#    endif
#endif

typedef void (*PeerPktHandler)(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData);

struct sLaika_peerPacketInfo
{
    PeerPktHandler handler;
    LAIKAPKT_SIZE size;
    bool variadic;
};

#define LAIKA_CREATE_PACKET_INFO(ID, HANDLER, SIZE, ISVARIADIC)                                    \
    [ID] = {.handler = HANDLER, .size = SIZE, .variadic = ISVARIADIC}

struct sLaika_peer
{
    struct sLaika_socket sock; /* DO NOT MOVE THIS. this member HAS TO BE FIRST so that typecasting
                                  sLaika_peer* to sLaika_sock* works as intended */
    uint8_t peerPub[crypto_kx_PUBLICKEYBYTES]; /* connected peer's public key */
    uint8_t inKey[crypto_kx_SESSIONKEYBYTES], outKey[crypto_kx_SESSIONKEYBYTES];
    char hostname[LAIKA_HOSTNAME_LEN], inet[LAIKA_INET_LEN], ipStr[LAIKA_IPSTR_LEN];
    uint8_t salt[LAIKA_HANDSHAKE_SALT_LEN];  /* salt passed from the cnc's handshake response */
    struct sLaika_pollList *pList;           /* pollList we're activeList in */
    struct sLaika_peerPacketInfo *packetTbl; /* const table to pull pkt data from */
    void *uData;                             /* data to be passed to pktHandler */
    LAIKAPKT_SIZE pktSize;                   /* current pkt size */
    LAIKAPKT_ID pktID;                       /* current pkt ID */
    PEERTYPE type;
    OSTYPE osType;
    int outStart;   /* index of pktID for out packet */
    int inStart;    /* index of pktID for in packet */
    bool useSecure; /* if true, peer will transmit/receive encrypted data using inKey & outKey */
};

struct sLaika_peer *laikaS_newPeer(struct sLaika_peerPacketInfo *packetTbl,
                                   struct sLaika_pollList *pList, pollFailEvent onPollFail,
                                   void *onPollFailUData, void *uData);
void laikaS_freePeer(struct sLaika_peer *peer);

void laikaS_setSalt(struct sLaika_peer *peer, uint8_t *salt);
void laikaS_genSalt(struct sLaika_peer *peer);

void laikaS_setSecure(struct sLaika_peer *peer, bool flag);
void laikaS_emptyOutPacket(struct sLaika_peer *peer,
                           LAIKAPKT_ID id); /* for sending packets with no body */
void laikaS_startOutPacket(struct sLaika_peer *peer, LAIKAPKT_ID id);
int laikaS_endOutPacket(struct sLaika_peer *peer);
void laikaS_startVarPacket(struct sLaika_peer *peer, LAIKAPKT_ID id);
int laikaS_endVarPacket(struct sLaika_peer *peer);
bool laikaS_handlePeerIn(struct sLaika_socket *sock);
bool laikaS_handlePeerOut(struct sLaika_socket *sock);

#endif