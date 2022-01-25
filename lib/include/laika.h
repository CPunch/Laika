#ifndef LAIKA_LAIKA_H
#define LAIKA_LAIKA_H

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define ARRAY_START 4

#ifdef DEBUG
#define LAIKA_DEBUG(...) printf(__VA_ARGS__);
#else
#define LAIKA_DEBUG(...)
#endif

/* for intellisense */
#ifndef LAIKA_VERSION_MAJOR
#define LAIKA_VERSION_MAJOR 0
#endif

#ifndef LAIKA_VERSION_MINOR
#define LAIKA_VERSION_MINOR 0
#endif

#endif