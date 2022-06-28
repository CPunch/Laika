#ifndef LAIKA_SHELL_H
#define LAIKA_SHELL_H

#include "laika.h"
#include "lpacket.h"

#define LAIKA_SHELL_TASK_DELTA 50

struct sLaika_bot;
struct sLaika_shell
{
    uint32_t id;
};

struct sLaika_shell *laikaB_newShell(struct sLaika_bot *bot, int cols, int rows, uint32_t id);
void laikaB_freeShell(struct sLaika_bot *bot, struct sLaika_shell *shell);

/* raw platform-dependent shell allocation */
struct sLaika_shell *laikaB_newRAWShell(struct sLaika_bot *bot, int cols, int rows, uint32_t id);
void laikaB_freeRAWShell(struct sLaika_bot *bot, struct sLaika_shell *shell);

/* handles reading & writing to shell pipes */
bool laikaB_readShell(struct sLaika_bot *bot, struct sLaika_shell *shell);
bool laikaB_writeShell(struct sLaika_bot *bot, struct sLaika_shell *shell, char *buf,
                       size_t length);

/* packet handlers */
void laikaB_handleShellOpen(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData);
void laikaB_handleShellClose(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData);
void laikaB_handleShellData(struct sLaika_peer *peer, LAIKAPKT_SIZE sz, void *uData);

void laikaB_shellTask(struct sLaika_taskService *service, struct sLaika_task *task,
                      clock_t currTick, void *uData);

#endif