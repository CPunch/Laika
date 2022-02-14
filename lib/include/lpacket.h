#ifndef LAIKA_PACKET_H
#define LAIKA_PACKET_H

#define LAIKA_MAGIC "LAI\x12"
#define LAIKA_MAGICLEN 4

#define LAIKA_MAX_PKTSIZE 4096

/* NONCE: randomly generated uint8_t[LAIKA_NONCESIZE] */

/* first handshake between peer & cnc works as so:
    - peer connects to cnc and sends a LAIKAPKT_HANDSHAKE_REQ with the peer's pubkey
    - after cnc receives LAIKAPKT_HANDSHAKE_REQ, all packets are encrypted
    - cnc responds with LAIKAPKT_HANDSHAKE_RES
    - if peer is an authenticated client (panel), LAIKAPKT_AUTHENTICATED_HANDSHAKE_REQ is then sent
*/

enum {
    LAIKAPKT_HANDSHAKE_REQ,
    /* layout of LAIKAPKT_HANDSHAKE_REQ:
    *   uint8_t laikaMagic[LAIKA_MAGICLEN];
    *   uint8_t majorVer;
    *   uint8_t minorVer;
    *   uint8_t pubKey[crypto_kx_PUBLICKEYBYTES]; -- freshly generated pubKey to encrypt decrypted nonce with
    */
    LAIKAPKT_HANDSHAKE_RES,
    /* layout of LAIKAPKT_HANDSHAKE_RES:
    *   uint8_t endian;
    */
    LAIKAPKT_AUTHENTICATED_HANDSHAKE_REQ,
    /* layout of LAIKAPKT_STAGE2_HANDSHAKE_REQ
    *   uint8_t peerType;
    */
    LAIKAPKT_AUTHENTICATED_ADD_PEER, /* notification that a peer has connected to the cnc */
    /* layout of LAIKAPKT_AUTHENTICATED_ADD_PEER
    *   uint8_t pubKey[crypto_kx_PUBLICKEYBYTES]; -- pubkey of said bot
    *   uint8_t peerType;
    *   -- reserved info later (machine info including hostname, OS, machineType, ip, etc.)
    */
    LAIKAPKT_AUTHENTICATED_RMV_PEER, /* notification that a peer has disconnected from the cnc */
    /* layout of LAIKAPKT_AUTHENTICATED_RMV_PEER
    *   uint8_t pubKey[crypto_kx_PUBLICKEYBYTES]; -- pubkey of said bot
    *   uint8_t peerType;
    */
    //LAIKAPKT_VARPKT_REQ,
    /* layout of LAIKAPKT_VARPKT_REQ:
    *   uint8_t pktID;
    *   uint16_t pktSize;
    */
    LAIKAPKT_MAXNONE
};

typedef uint8_t LAIKAPKT_ID;
typedef uint16_t LAIKAPKT_SIZE;

#endif