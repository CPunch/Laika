#ifndef LAIKA_LAIKA_H
#define LAIKA_LAIKA_H

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "lconfig.h"

#ifdef DEBUG
# define LAIKA_DEBUG(...) printf("[~] " __VA_ARGS__); fflush(stdout);
#else
# define LAIKA_DEBUG(...) ((void)0) /* no op */
#endif

#ifndef _MSC_VER
# define LAIKA_FORCEINLINE __attribute__((always_inline)) inline
#else
# define LAIKA_FORCEINLINE __forceinline
#endif

LAIKA_FORCEINLINE int MIN(int a, int b) {
    return a < b ? a : b;
}

LAIKA_FORCEINLINE int MAX(int a, int b) {
    return a > b ? a : b;
}

struct sLaika_peer;
struct sLaika_socket;
struct sLaika_pollList;
struct sLaika_task;
struct sLaika_taskService;
struct sLaika_content;
struct sLaika_contentContext;

#endif