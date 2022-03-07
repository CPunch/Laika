#include "lmem.h"
#include "scmd.h"
#include "sterm.h"

#define KEY_ESCAPE  0x001b
#define KEY_ENTER   0x000a
#define KEY_BACKSPACE 0x007f
#define KEY_UP      0x0105
#define KEY_DOWN    0x0106
#define KEY_LEFT    0x0107
#define KEY_RIGHT   0x0108

#define cursorForward(x) printf("\033[%dC", (x))
#define cursorBackward(x) printf("\033[%dD", (x))
#define clearLine() printf("\033[2K")

struct termios orig_termios;
char *cmd, *prompt = "$> ";
int cmdCount = 0, cmdCap = 4, cmdCursor = 0;

void shellT_conioTerm(void) {
    struct termios new_termios;

    /* take two copies - one for now, one for later */
    tcgetattr(0, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));

    /* register cleanup handler, and set the new terminal mode */
    atexit(shellT_resetTerm);
    new_termios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
}

void shellT_resetTerm(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

void shellT_printf(const char *format, ...) {
    va_list args;

    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    fflush(stdout);
}

/* waits for input for timeout. returns true if input is ready to be read, false if no events */
bool shellT_waitForInput(int timeout) {
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

int shellT_readRawInput(uint8_t *buf, size_t max) {
    return read(STDIN_FILENO, buf, max);
}

void shellT_writeRawOutput(uint8_t *buf, size_t sz) {
    write(STDOUT_FILENO, buf, sz);
    fflush(stdout);
}

void shellT_getTermSize(int *col, int *row) {
    struct winsize ws;
    ioctl(STDIN_FILENO, TIOCGWINSZ, &ws);

    *col = ws.ws_col;
    *row = ws.ws_row;
}

char shellT_getch(void) {
    int r;
    char in;

    if ((r = shellT_readRawInput(&in, 1)) > 0) {
        return in;
    } else {
        return r;
    }
}

int shellT_kbesc(void) {
    int c;

    /* if no event waiting, it's KEY_ESCAPE */
    if (!shellT_waitForInput(0))
        return KEY_ESCAPE;

    if ((c = shellT_getch()) == '[') {
        switch (shellT_getch()) {
            case 'A': c = KEY_UP; break;
            case 'B': c = KEY_DOWN; break;
            case 'C': c = KEY_RIGHT; break;
            case 'D': c = KEY_LEFT; break;
            default: c = 0; break;
        }
    } else {
        c = 0;
    }

    /* unrecognized key? consume until there's no event */
    if (c == 0) {
        while (shellT_waitForInput(0)) shellT_getch();
    }

    return c;
}

int shellT_kbget(void) {
    char c = shellT_getch();
    return (c == KEY_ESCAPE) ? shellT_kbesc() : c;
}

void shellT_printPrompt(void) {
    clearLine();
    shellT_printf("\r%s%.*s", prompt, cmdCount, (cmd ? cmd : ""));
    if (cmdCount > cmdCursor)
        cursorBackward(cmdCount-cmdCursor);
    fflush(stdout);
}

void shellT_setPrompt(char *_prompt) {
    prompt = _prompt;
}

void shellT_addChar(tShell_client *client, int c) {
    int i;

    switch (c) {
        case KEY_BACKSPACE:
            if (cmdCursor > 0) {
                laikaM_rmvarray(cmd, cmdCount, (cmdCursor-1), 1);
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
        case KEY_UP: case KEY_DOWN: break; /* ignore these */
        default:
            laikaM_growarray(char, cmd, 1, cmdCount, cmdCap);
            laikaM_insertarray(cmd, cmdCount, cmdCursor, 1);
            cmd[cmdCursor++] = c;
            shellT_printPrompt();
    }
}