#include "lmem.h"
#include "lpacket.h"
#include "speer.h"
#include "sterm.h"

tShell_peer *shellP_newPeer(PEERTYPE type, OSTYPE osType, uint8_t *pubKey, char *hostname, char *inet, char *ipv4) {
    tShell_peer *peer = (tShell_peer*)laikaM_malloc(sizeof(tShell_peer));
    peer->type = type;
    peer->osType = osType;

    /* copy pubKey to peer's pubKey */
    memcpy(peer->pub, pubKey, crypto_kx_PUBLICKEYBYTES);

    /* copy hostname & ipv4 */
    memcpy(peer->hostname, hostname, LAIKA_HOSTNAME_LEN);
    memcpy(peer->inet, inet, LAIKA_IPV4_LEN);
    memcpy(peer->ipv4, ipv4, LAIKA_IPV4_LEN);

    /* restore NULL terminators */
    peer->hostname[LAIKA_HOSTNAME_LEN-1] = '\0';
    peer->inet[LAIKA_INET_LEN-1] = '\0';
    peer->ipv4[LAIKA_IPV4_LEN-1] = '\0';

    return peer;
}

void shellP_freePeer(tShell_peer *peer) {
    laikaM_free(peer);
}

char *shellP_typeStr(tShell_peer *peer) {
    switch (peer->type) {
        case PEER_BOT: return "Bot";
        case PEER_CNC: return "CNC";
        case PEER_AUTH: return "Auth";
        default: return "err";
    }
}

char *shellP_osTypeStr(tShell_peer *peer) {
    switch (peer->osType) {
        case OS_WIN: return "Windows";
        case OS_LIN: return "Linux";
        default: return "unkn";
    }
}

void shellP_printInfo(tShell_peer *peer) {
    char buf[128];

    sodium_bin2hex(buf, sizeof(buf), peer->pub, crypto_kx_PUBLICKEYBYTES);
    shellT_printf("\t%s@%s\n\tTYPE: %s\n\tOS: %s\n\tPUBKEY: %s\n\tINET: %s\n", peer->hostname, peer->ipv4, shellP_typeStr(peer), shellP_osTypeStr(peer), buf, peer->inet);
}