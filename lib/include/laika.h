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
#define LAIKA_DEBUG(...) printf("[~] " __VA_ARGS__);
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

/* for testing!! make sure you pass your generated keypair to cmake */
#ifndef LAIKA_PUBKEY
#define LAIKA_PUBKEY "997d026d1c65deb6c30468525132be4ea44116d6f194c142347b67ee73d18814"
#endif

#ifndef LAIKA_PRIVKEY
#define LAIKA_PRIVKEY "1dbd33962f1e170d1e745c6d3e19175049b5616822fac2fa3535d7477957a841"
#endif

#endif