#ifndef LAIKA_ERROR_H
#define LAIKA_ERROR_H

#include <stdio.h>
#include <setjmp.h>

/* defines errorstack size */
#define LAIKA_MAXERRORS 32

/* DO NOT RETURN/GOTO/BREAK or otherwise skip LAIKA_TRYEND */
#define LAIKA_TRY if (setjmp(eLaika_errStack[++eLaika_errIndx]) == 0) {
#define LAIKA_CATCH } else {
#define LAIKA_TRYEND } --eLaika_errIndx;

/* if eLaika_errIndx is >= 0, we have a safe spot to jump too if an error is thrown */
#define LAIKA_ISPROTECTED (eLaika_errIndx >= 0)

/* LAIKA_ERROR(printf args):
        if called after a LAIKA_TRY block will jump to the previous LAIKA_CATCH/LAIKA_TRYEND block,
    otherwise program is exit()'d. if DEBUG is defined printf is called with passed args, else
    arguments are ignored.
*/
#ifndef DEBUG
#define LAIKA_ERROR(...) { \
    if (LAIKA_ISPROTECTED) \
        longjmp(eLaika_errStack[eLaika_errIndx], 1); \
    else \
        exit(1); \
}
#define LAIKA_WARN(...)
#else
#define LAIKA_ERROR(...) { \
    printf("[ERROR] : " __VA_ARGS__); \
    if (LAIKA_ISPROTECTED) \
        longjmp(eLaika_errStack[eLaika_errIndx], 1); \
    else \
        exit(1); \
}

#define LAIKA_WARN(...) \
    printf("[WARN] : " __VA_ARGS__);
#endif

extern int eLaika_errIndx;
extern jmp_buf eLaika_errStack[LAIKA_MAXERRORS];

#endif