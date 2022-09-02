#include "sterm.h"

#include "lmem.h"
#include "scmd.h"

#define KEY_ESCAPE        0x001b
#define KEY_ENTER         0x000a
#define KEY_BACKSPACE     0x007f
#define KEY_UP            0x0105
#define KEY_DOWN          0x0106
#define KEY_LEFT          0x0107
#define KEY_RIGHT         0x0108

#define cursorForward(x)  printf("\033[%dC", (x))
#define cursorBackward(x) printf("\033[%dD", (x))
#define clearLine()       printf("\033[2K")

/* =================================[[ DEPRECATED ARRAY API ]]================================== */

/*
    this whole target needs to be rewritten, so these macros have been embedded here until a
    rewrite can be done. this is the only section of the entire codebase that relies too heavily
    on these to quickly exchange into the vector api equivalent. there's technically a memory leak
    here since the array is never free'd, but since the array is expected to live the entire
    lifetime of the program it's safe to leave as-is for now.
*/

#define laikaM_growarray(type, buf, needed, count, capacity)                                       \
    if (count + needed >= capacity || buf == NULL) {                                               \
        capacity = (capacity + needed) * GROW_FACTOR;                                              \
        buf = (type *)laikaM_realloc(buf, sizeof(type) * capacity);                                \
    }

/* moves array elements above indx down by numElem, removing numElem elements at indx */
#define laikaM_rmvarray(buf, count, indx, numElem)                                                 \
    do {                                                                                           \
        int _i, _sz = ((count - indx) - numElem);                                                  \
        for (_i = 0; _i < _sz; _i++)                                                               \
            buf[indx + _i] = buf[indx + numElem + _i];                                             \
        count -= numElem;                                                                          \
    } while (0);

/* moves array elements above indx up by numElem, inserting numElem elements at indx */
#define laikaM_insertarray(buf, count, indx, numElem)                                              \
    do {                                                                                           \
        int _i;                                                                                    \
        for (_i = count; _i > indx; _i--)                                                          \
            buf[_i] = buf[_i - 1];                                                                 \
        count += numElem;                                                                          \
    } while (0);

struct termios orig_termios;
char *cmd = NULL, *prompt = "$> ";
int cmdCount = 0, cmdCap = 4, cmdCursor = 0;

void shellT_conioTerm(void)
{
    struct termios new_termios;

    /* take two copies - one for now, one for later */
    tcgetattr(0, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));

    /* register cleanup handler, and set the new terminal mode */
    atexit(shellT_resetTerm);
    new_termios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
}

void shellT_resetTerm(void)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

const char *shellT_getForeColor(TERM_COLOR col)
{
    switch (col) {
    case TERM_BLACK:
        return "\033[30m";
        break;
    case TERM_RED:
        return "\033[31m";
        break;
    case TERM_GREEN:
        return "\033[32m";
        break;
    case TERM_YELLOW:
        return "\033[33m";
        break;
    case TERM_BLUE:
        return "\033[34m";
        break;
    case TERM_MAGENTA:
        return "\033[35m";
        break;
    case TERM_CYAN:
        return "\033[36m";
        break;
    case TERM_WHITE:
        return "\033[37m";
        break;
    case TERM_BRIGHT_BLACK:
        return "\033[90m";
        break;
    case TERM_BRIGHT_RED:
        return "\033[91m";
        break;
    case TERM_BRIGHT_GREEN:
        return "\033[92m";
        break;
    case TERM_BRIGHT_YELLOW:
        return "\033[93m";
        break;
    case TERM_BRIGHT_BLUE:
        return "\033[94m";
        break;
    case TERM_BRIGHT_MAGENTA:
        return "\033[95m";
        break;
    case TERM_BRIGHT_CYAN:
        return "\033[96m";
        break;
    case TERM_BRIGHT_WHITE:
    default:
        return "\033[97m";
        break;
    }
}

void shellT_printf(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    fflush(stdout);
}

/* waits for input for timeout. returns true if input is ready to be read, false if no events */
bool shellT_waitForInput(int timeout)
{
    struct timeval tv;
    fd_set fds;

    /* setup stdin file descriptor */
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);

    /* wait for read events on STDIN_FILENO for timeout period */
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
    return select(1, &fds, NULL, NULL, &tv) > 0;
}

int shellT_readRawInput(uint8_t *buf, size_t max)
{
    return read(STDIN_FILENO, buf, max);
}

void shellT_writeRawOutput(uint8_t *buf, size_t sz)
{
    write(STDOUT_FILENO, buf, sz);
    fflush(stdout);
}

void shellT_getTermSize(int *col, int *row)
{
    struct winsize ws;
    ioctl(STDIN_FILENO, TIOCGWINSZ, &ws);

    *col = ws.ws_col;
    *row = ws.ws_row;
}

char shellT_getch(void)
{
    int r;
    char in;

    if ((r = shellT_readRawInput((uint8_t *)&in, 1)) > 0) {
        return in;
    } else {
        return r;
    }
}

int shellT_kbesc(void)
{
    int c;

    /* if no event waiting, it's KEY_ESCAPE */
    if (!shellT_waitForInput(0))
        return KEY_ESCAPE;

    if ((c = shellT_getch()) == '[') {
        switch (shellT_getch()) {
        case 'A':
            c = KEY_UP;
            break;
        case 'B':
            c = KEY_DOWN;
            break;
        case 'C':
            c = KEY_RIGHT;
            break;
        case 'D':
            c = KEY_LEFT;
            break;
        default:
            c = 0;
            break;
        }
    } else {
        c = 0;
    }

    /* unrecognized key? consume until there's no event */
    if (c == 0) {
        while (shellT_waitForInput(0))
            shellT_getch();
    }

    return c;
}

int shellT_kbget(void)
{
    char c = shellT_getch();
    return (c == KEY_ESCAPE) ? shellT_kbesc() : c;
}

void shellT_printPrompt(void)
{
    clearLine();
    shellT_printf("\r%s%.*s", prompt, cmdCount, (cmd ? cmd : ""));
    if (cmdCount > cmdCursor)
        cursorBackward(cmdCount - cmdCursor);
    fflush(stdout);
}

void shellT_setPrompt(char *_prompt)
{
    prompt = _prompt;
}

/* covers every non-controller related ascii characters (ignoring the extended character range) */
bool isAscii(int c)
{
    return c >= ' ' && c <= '~';
}

void shellT_addChar(tShell_client *client, int c)
{
    int i;

    switch (c) {
    case KEY_BACKSPACE:
        if (cmdCursor > 0) {
            laikaM_rmvarray(cmd, cmdCount, (cmdCursor - 1), 1);
            cmdCursor--;
            shellT_printPrompt();
        }
        break;
    case KEY_LEFT:
        if (cmdCursor > 0) {
            cursorBackward(1);
            --cmdCursor;
            fflush(stdout);
        }
        break;
    case KEY_RIGHT:
        if (cmdCursor < cmdCount) {
            cursorForward(1);
            cmdCursor++;
            fflush(stdout);
        }
        break;
    case KEY_ENTER:
        if (cmdCount > 0) {
            cmd[cmdCount] = '\0';
            cmdCount = 0;
            cmdCursor = 0;
            shellS_runCmd(client, cmd);
            shellT_printPrompt();
        }
        break;
    case KEY_UP:
    case KEY_DOWN:
        break; /* ignore these */
    default:
        /* we only want to accept valid input, just ignore non-ascii characters */
        if (!isAscii(c))
            return;

        laikaM_growarray(char, cmd, 1, cmdCount, cmdCap);
        laikaM_insertarray(cmd, cmdCount, cmdCursor, 1);
        cmd[cmdCursor++] = c;
        shellT_printPrompt();
    }
}