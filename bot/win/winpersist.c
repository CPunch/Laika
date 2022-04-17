/* platform specific code for achieving persistence on windows */

#include <windows.h>
#include "persist.h"
#include "lconfig.h"

/*      we want a semi-random mutex that is stable between similar builds,
*   so we use the GIT_VERSION as our mutex :D */
#define LAIKA_MUTEX LAIKA_VERSION_COMMIT ".0"

/* check if laika is running as super-user */
bool laikaB_checkRoot() {
    return true; /* stubbed for now */
}

/* mark that laika is currently running */
void laikaB_markRunning() {
    /* stubbed */
}

/* unmark that laika is currently running */
void laikaB_unmarkRunning() {
    /* stubbed */
}

/* try to gain persistance on machine */
void laikaB_tryPersist() {
    /* stubbed */
}

/* try to gain root */
void laikaB_tryRoot() {
    /* stubbed */
}