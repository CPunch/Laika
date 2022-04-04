#ifndef LAIKA_PERSIST_H
#define LAIKA_PERSIST_H

#include <stdbool.h>

/* check if laika is already running */
bool laikaB_checkRunning(void);

/* check if laika is already installed on current machine */
bool laikaB_checkPersist(void);

/* check if laika is running as super-user */
bool laikaB_checkRoot(void);

/* mark that laika is currently running */
void laikaB_markRunning(void);

/* unmark that laika is currently running */
void laikaB_unmarkRunning(void);

/* try to gain persistance on machine */
void laikaB_tryPersist(void);

/* try to gain root */
void laikaB_tryRoot(void);

#endif