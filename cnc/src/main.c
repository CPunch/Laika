#include <stdio.h>

#include "ltask.h"
#include "cnc.h"

struct sLaika_taskService tService;

/*void debugTask(struct sLaika_taskService *service, struct sLaika_task *task, clock_t currTick, void *uData) {
    printf("hello !\n");
}*/

int main(int argv, char **argc) {
    struct sLaika_cnc *cnc = laikaC_newCNC(13337);

    laikaT_initTaskService(&tService);
    /*laikaT_newTask(&tService, 2000, debugTask, NULL);*/

    while (true) {
        laikaC_pollPeers(cnc, laikaT_timeTillTask(&tService));
        laikaT_pollTasks(&tService);
    }

    laikaC_freeCNC(cnc);
    LAIKA_DEBUG("cnc killed\n");
    return 0;
}