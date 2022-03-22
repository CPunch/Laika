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

typedef enum {
    TERM_BLACK,
    TERM_RED,
    TERM_GREEN,
    TERM_YELLOW,
    TERM_BLUE,
    TERM_MAGENTA,
    TERM_CYAN,
    TERM_WHITE,
    TERM_BRIGHT_BLACK,
    TERM_BRIGHT_RED,
    TERM_BRIGHT_GREEN,
    TERM_BRIGHT_YELLOW,
    TERM_BRIGHT_BLUE,
    TERM_BRIGHT_MAGENTA,
    TERM_BRIGHT_CYAN,
    TERM_BRIGHT_WHITE
} TERM_COLOR;

#define PRINTTAG(color) shellT_printf("\r%s[~]%s ", shellT_getForeColor(color), shellT_getForeColor(TERM_BRIGHT_WHITE))

#define PRINTINFO(...) do { \
    PRINTTAG(TERM_BRIGHT_YELLOW); \
    shellT_printf(__VA_ARGS__); \
} while(0);

#define PRINTSUCC(...) do { \
    PRINTTAG(TERM_BRIGHT_GREEN); \
    shellT_printf(__VA_ARGS__); \
} while(0);

#define PRINTERROR(...) do { \
    PRINTTAG(TERM_BRIGHT_RED); \
    shellT_printf(__VA_ARGS__); \
} while(0);

void shellT_conioTerm(void);
void shellT_resetTerm(void);
const char *shellT_getForeColor(TERM_COLOR);
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