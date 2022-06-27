#ifndef SHELLCMD_H
#define SHELLCMD_H

#include "sclient.h"

#include <string.h>

typedef void (*shellCmdCallback)(tShell_client *client, int args, char *argc[]);

typedef struct sShell_cmdDef
{
    const char *cmd;
    const char *help;
    const char *syntax;
    shellCmdCallback callback;
} tShell_cmdDef;

extern tShell_cmdDef shellS_cmds[];

void shellS_initCmds(void);
void shellS_cleanupCmds(void);

void shellS_runCmd(tShell_client *client, char *cmd);

#endif