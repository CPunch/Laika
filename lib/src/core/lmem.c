#include "core/lmem.h"

#include "core/lerror.h"

void *laikaM_realloc(void *buf, size_t sz)
{
    void *newBuf;

    /* are we free'ing the buffer? */
    if (sz == 0) {
        free(buf);
        return NULL;
    }

    /* if NULL is passed, realloc() acts like malloc() */
    if ((newBuf = realloc(buf, sz)) == NULL)
        LAIKA_ERROR("failed to allocate memory!\n");

    return newBuf;
}

bool laikaM_isBigEndian(void)
{
    union
    {
        uint32_t i;
        uint8_t c[4];
    } _indxint = {0xDEADB33F};

    return _indxint.c[0] == 0xDE;
}

void laikaM_reverse(uint8_t *buf, size_t sz)
{
    int k;

    /* swap bytes, reversing the buffer */
    for (k = 0; k < (sz / 2); k++) {
        uint8_t tmp = buf[k];
        buf[k] = buf[sz - k - 1];
        buf[sz - k - 1] = tmp;
    }
}