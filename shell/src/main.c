#include "core/ini.h"
#include "sclient.h"
#include "sterm.h"

#include <stdio.h>

#define STRING(x)      #x
#define MACROLITSTR(x) STRING(x)

const char *LOGO =
    "\n        ██╗      █████╗ ██╗██╗  ██╗ █████╗\n        ██║     ██╔══██╗██║██║ ██╔╝██╔══██╗\n   "
    "     ██║     ███████║██║█████╔╝ ███████║\n        ██║     ██╔══██║██║██╔═██╗ ██╔══██║\n       "
    " ███████╗██║  ██║██║██║  ██╗██║  ██║\n        ╚══════╝╚═╝  ╚═╝╚═╝╚═╝  ╚═╝╚═╝  ╚═╝";


#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

static int iniHandler(void *user, const char *section, const char *name, const char *value)
{
    tShell_client *client = (tShell_client *)user;

    if (MATCH("auth", "public-key")) {
        shellC_loadKeys(client, value, NULL);
        PRINTINFO("Auth pubkey: %s\n", value);
    } else if (MATCH("auth", "private-key")) {
        shellC_loadKeys(client, NULL, value);
    } else {
        return 0; /* unknown section/name, error */
    }
    return 1;
}

#undef MATCH

bool loadConfig(tShell_client *client, char *config)
{
    int iniRes;

    printf("Loading config file '%s'...\n", config);
    if ((iniRes = ini_parse(config, iniHandler, (void *)client)) < 0) {
        switch (iniRes) {
        case -1:
            printf("Couldn't load config file '%s'!\n", config);
            break;
        case -2:
            printf("Memory allocation error :/\n");
            break;
        default:
            printf("Parser error on line %d in config file '%s'!\n", iniRes, config);
        }
        return false;
    }

    return true;
}

int main(int argv, char *argc[])
{
    tShell_client client;
    char *configFile = "shell.ini";
    bool printPrompt = false;

    shellT_printf("%s%s\n%s", shellT_getForeColor(TERM_BRIGHT_RED), LOGO,
                  shellT_getForeColor(TERM_BRIGHT_WHITE));

    shellT_printf(
        "       made with %s<3%s by CPunch - %s\n\nType 'help' for a list of commands\n\n",
        shellT_getForeColor(TERM_BRIGHT_RED), shellT_getForeColor(TERM_BRIGHT_WHITE),
        "v" MACROLITSTR(LAIKA_VERSION_MAJOR) "."
        MACROLITSTR(LAIKA_VERSION_MINOR) "-" LAIKA_VERSION_COMMIT);

    shellC_init(&client);

    /* load config file */
    if (argv >= 2)
        configFile = argc[1];

    if (!loadConfig(&client, configFile))
        return 1;

    shellC_connectToCNC(&client, LAIKA_CNC_IP, LAIKA_CNC_PORT);

    shellT_conioTerm();
    while (laikaS_isAlive((&client.peer->sock))) {
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
    shellT_resetTerm();

    shellC_cleanup(&client);
    PRINTERROR("Connection closed\n");
    return 0;
}