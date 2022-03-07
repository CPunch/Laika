#ifndef SHELLTERM_H
#define SHELLTERM_H

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <stdbool.h>

#include "sclient.h"

void shellT_conioTerm(void);
void shellT_resetTerm(void);
void shellT_printf(const char *format, ...);

/* waits for input for timeout (in ms). returns true if input is ready to be read, false if no events */
bool shellT_waitForInput(int timeout);
int shellT_readRawInput(uint8_t *buf, size_t max);
void shellT_writeRawOutput(uint8_t *buf, size_t sz);
void shellT_getTermSize(int *col, int *row);
char shellT_getch(void);
int shellT_kbget(void);
void shellT_printPrompt(void);
void shellT_setPrompt(char *prompt);
void shellT_addChar(tShell_client *client, int c); /* processes input, moving cursor, adding char to cmd, etc. */

#endif