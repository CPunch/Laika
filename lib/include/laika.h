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

#endif