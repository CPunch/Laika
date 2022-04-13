#ifndef LAIKA_TASK_H
#define LAIKA_TASK_H

#include <time.h>

struct sLaika_task;
struct sLaika_taskService;
typedef void (*taskCallback)(struct sLaika_taskService *service, struct sLaika_task *task, clock_t currTick, void *uData);

struct sLaika_task {
    struct sLaika_task *next;
    taskCallback callback;
    void *uData;
    long scheduled;
    int delta;
};

struct sLaika_taskService {
    struct sLaika_task *headTask;
};

void laikaT_initTaskService(struct sLaika_taskService *service);
void laikaT_cleanTaskService(struct sLaika_taskService *service);

struct sLaika_task *laikaT_newTask(struct sLaika_taskService *service, int delta, taskCallback callback, void *uData);
void laikaT_delTask(struct sLaika_taskService *service, struct sLaika_task *task);

void laikaT_pollTasks(struct sLaika_taskService *service);

/* will return the delta time in ms till the next event. -1 for no tasks scheduled */
int laikaT_timeTillTask(struct sLaika_taskService *service);

long laikaT_getTime(void);

#endif