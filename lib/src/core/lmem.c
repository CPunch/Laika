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
