#include "lmem.h"
#include "ltask.h"

/* this is the only reason C11 support is needed, i cba to write windows/linux specific stuff to get the current time in ms
    also side note: microsoft? more like micropenis */
long getTime() {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return ts.tv_sec*1000 + ts.tv_nsec/1000000; /* convert nanoseconds to milliseconds */
}

void laikaT_initTaskService(struct sLaika_taskService *service) {
    service->headTask = NULL;
}

void laikaT_cleanTaskService(struct sLaika_taskService *service) {
    struct sLaika_task *curr = service->headTask, *last;

    /* walk though tasks, freeing all */
    while (curr != NULL) {
        last = curr;
        curr = curr->next;
        laikaM_free(last);
    }
}

struct sLaika_task *laikaT_newTask(struct sLaika_taskService *service, int delta, taskCallback callback, void *uData) {
    struct sLaika_task *curr = service->headTask, *last = NULL, *task = laikaM_malloc(sizeof(struct sLaika_task));

    task->callback = callback;
    task->uData = uData;
    task->scheduled = getTime() + delta;

    /* search list for event for which we're scheduled before */
    while (curr != NULL && curr->scheduled < task->scheduled) {
        last = curr;
        curr = curr->next;
    }

    /* if we stopped at the first head item, set it */
    if (curr == service->headTask) {
        task->next = service->headTask;
        service->headTask = task;
    } else {
        /* add to list */
        task->next = curr;
        if (last)
            last->next = task;
    }
}

void laikaT_delTask(struct sLaika_taskService *service, struct sLaika_task *task) {
    struct sLaika_task *curr = service->headTask, *last = NULL;

    if (task == service->headTask) {
        service->headTask = task->next;
    } else {
        /* find in list */
        while (curr != task) {
            last = curr;
            curr = curr->next;
        }

        /* unlink */
        last->next = task->next;
    }

    laikaM_free(task);
}

void laikaT_pollTasks(struct sLaika_taskService *service) {
    struct sLaika_task *last, *curr = service->headTask;
    clock_t currTick = getTime();

    /* run each task, list is already sorted from closest scheduled task to furthest */
    while (curr != NULL && curr->scheduled < currTick) { /* if scheduled time is greater than currTime, all events that follow are also not scheduled yet */
        /* dispatch task callback */
        curr->callback(service, curr, currTick, curr->uData);
        
        /* walk to next task and free */
        last = curr;
        curr = curr->next;
        laikaT_delTask(service, last);
    }
}

/* will return the delta time in ms till the next event. -1 for no tasks scheduled */
int laikaT_timeTillTask(struct sLaika_taskService *service) {
    if (service->headTask)
        return service->headTask->scheduled - getTime();
    else
        return -1; /* no tasks scheduled */
}