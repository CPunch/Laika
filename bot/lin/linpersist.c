/* platform specific code for achieving persistence on linux */

/* this is only used to check if another instance of laika is currently running */
#define LAIKA_RESERVED_PORT 32876

#include "persist.h"
#include "lsocket.h"
#include "lerror.h"

static struct sLaika_socket laikaB_markerPort;

/* check if laika is already running */
bool laikaB_checkRunning() {
    return true; /* stubbed for now */
}

/* check if laika is already installed on current machine */
bool laikaB_checkPersist() {
    return true; /* stubbed for now */
}

/* check if laika is running as super-user */
bool laikaB_checkRoot() {
    return true; /* stubbed for now */
}

/* mark that laika is currently running */
void laikaB_markRunning() {
#ifndef DEBUG
    LAIKA_TRY
        laikaS_initSocket(&laikaB_markerPort, NULL, NULL, NULL, NULL);
        laikaS_bind(&laikaB_markerPort, LAIKA_RESERVED_PORT);
    LAIKA_CATCH
        LAIKA_DEBUG("Failed to bind marker port, laika is already running!\n");
        exit(0);
    LAIKA_TRYEND
#endif
}

/* unmark that laika is currently running */
void laikaB_unmarkRunning() {
#ifndef DEBUG
    laikaS_kill(&laikaB_markerPort);
#endif
}

/* try to gain persistance on machine */
void laikaB_tryPersist() {
    /* stubbed */
}

/* try to gain root */
void laikaB_tryRoot() {
    /* stubbed */
}