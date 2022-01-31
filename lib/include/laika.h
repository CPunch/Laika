#ifndef LAIKA_LAIKA_H
#define LAIKA_LAIKA_H

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "lconfig.h"

#define ARRAY_START 4

#ifdef DEBUG
#define LAIKA_DEBUG(...) printf("[~] " __VA_ARGS__); fflush(stdout);
#else
#define LAIKA_DEBUG(...)
#endif

#endif