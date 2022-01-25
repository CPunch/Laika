#ifndef LAIKA_MEM_H
#define LAIKA_MEM_H

#include "laika.h"

#define GROW_FACTOR 2

#define laikaM_malloc(sz) laikaM_realloc(NULL, sz)
#define laikaM_free(buf) laikaM_realloc(buf, 0)

#define laikaM_growarray(type, buf, count, capacity) \
    if (count >= capacity || buf == NULL) { \
        capacity *= GROW_FACTOR; \
        buf = (type*)cosmoM_realloc(buf, sizeof(type)*capacity); \
    }

/* moves array elements above indx down by numElem, removing numElem elements at indx */ 
#define laikaM_rmvarray(type, buf, count, indx, numElem) { \
    memmove(&buf[indx], &buf[indx+numElem], ((count-indx)-numElem)*sizeof(type)); \
    count -= numElem; \
}

void *laikaM_realloc(void *buf, size_t sz);

#endif