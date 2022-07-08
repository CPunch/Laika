#ifndef LAIKA_OBF_H
#define LAIKA_OBF_H

#include "laika.h"

#ifdef _WIN32
#    include <windows.h>

/* WINAPI types */
typedef HINSTANCE(WINAPI *_ShellExecuteA)(HWND, LPCSTR, LPCSTR, LPCSTR, LPCSTR, INT);

extern _ShellExecuteA oShellExecuteA;
#endif

void laikaO_init();

#endif