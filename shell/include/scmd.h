#ifndef SHELLCMD_H
#define SHELLCMD_H

#include <string.h>

#include "sclient.h"

typedef void (*shellCmdCallback)(tShell_client *client, int args, char *argc[]);

typedef struct sShell_cmdDef {
    const char *cmd;
    const char *help;
    shellCmdCallback callback;
} tShell_cmdDef;

extern tShell_cmdDef shellS_cmds[];

void shellS_initCmds(void);
void shellS_cleanupCmds(void);

void shellS_runCmd(tShell_client *client, char *cmd);

#endif