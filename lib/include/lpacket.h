#ifndef LAIKA_PACKET_H
#define LAIKA_PACKET_H

#define LAIKA_MAGIC "LAI\x12"
#define LAIKA_MAGICLEN 4

#define LAIKA_MAX_PKTSIZE 4096

#define LAIKA_NONCESIZE 16

enum {
    LAIKAPKT_HANDSHAKE_REQ,
    /* layout of LAIKAPKT_HANDSHAKE_REQ:
    *   uint8_t laikaMagic[LAIKA_MAGICLEN];
    *   uint8_t majorVer;
    *   uint8_t minorVer;
    *   uint8_t encNonce[crypto_box_SEALBYTES + LAIKA_NONCESIZE]; -- encrypted using shared pubKey
    *   uint8_t pubKey[crypto_box_PUBLICKEYBYTES]; -- freshly generated pubKey to encrypt decrypted nonce with
    */
    LAIKAPKT_HANDSHAKE_RES,
    /* layout of LAIKAPKT_HANDSHAKE_RES:
    *   uint8_t endian;
    *   uint8_t reEncryptedNonce[crypto_box_SEALBYTES + LAIKA_NONCESIZE]; -- encrypted using received pubKey from LAIKAPKT_AUTH_REQ pkt
    */
    LAIKAPKT_VARPKT_REQ,
    LAIKAPKT_MAXNONE
};

typedef uint8_t LAIKAPKT_ID;
typedef uint16_t LAIKAPKT_SIZE;

#endif