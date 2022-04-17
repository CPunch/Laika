/* platform specific code for achieving persistence on windows */

#include <windows.h>

#include "persist.h"
#include "lconfig.h"
#include "lerror.h"

/*      we want a semi-random mutex that is stable between similar builds,
*   so we use the GIT_VERSION as our mutex :D */
#define LAIKA_MUTEX LAIKA_VERSION_COMMIT ".0"

HANDLE laikaB_mutex;

/* check if laika is running as super-user */
bool laikaB_checkRoot() {
    return true; /* stubbed for now */
}

/* mark that laika is currently running */
void laikaB_markRunning() {
    laikaB_mutex = OpenMutex(MUTEX_ALL_ACCESS, false, TEXT(LAIKA_MUTEX));

    if (!laikaB_mutex) {
        /* mutex doesn't exist, we're the only laika process :D */
        laikaB_mutex = CreateMutex(NULL, 0, TEXT(LAIKA_MUTEX));
    } else {
        LAIKA_ERROR("Mutex already exists!\n");
    }
}

/* unmark that laika is currently running */
void laikaB_unmarkRunning() {
    ReleaseMutex(laikaB_mutex);
}

/* try to gain persistance on machine */
void laikaB_tryPersist() {
    /* stubbed */
}

/* try to gain root */
void laikaB_tryRoot() {
    /* stubbed */
}