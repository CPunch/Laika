#include <stdio.h>

#include "sclient.h"
#include "sterm.h"

int main(int argv, char *argc[]) {
    tShell_client client;
    bool printPrompt = false;

    shellC_init(&client);
    shellC_connectToCNC(&client, "127.0.0.1", "13337");

    shellT_conioTerm();
    while(laikaS_isAlive((&client.peer->sock))) {
        /* poll for 50ms */
        if (!shellC_poll(&client, 50)) {
            /* check if we have input! */
            if (shellT_waitForInput(0))
                shellT_addChar(&client, shellT_kbget());

            /* no prompt shown? show it */
            if (!printPrompt) {
                printPrompt = true;

                shellT_printPrompt();
            }
        } else {
            printPrompt = false;
        }
    }

    shellC_cleanup(&client);
    return 0;
}