#ifndef LAIKA_MEM_H
#define LAIKA_MEM_H

#include "laika.h"

#define GROW_FACTOR 2

#define laikaM_malloc(sz) laikaM_realloc(NULL, sz)
#define laikaM_free(buf) laikaM_realloc(buf, 0)

#define laikaM_growarray(type, buf, needed, count, capacity) \
    if (count + needed >= capacity || buf == NULL) { \
        capacity = (capacity + needed) * GROW_FACTOR; \
        buf = (type*)laikaM_realloc(buf, sizeof(type)*capacity); \
    }

/* moves array elements above indx down by numElem, removing numElem elements at indx */ 
#define laikaM_rmvarray(type, buf, count, indx, numElem) { \
    int _i, _sz = ((count-indx)-numElem); \
    for (_i = 0; _i < _sz; _i++) \
        buf[indx+_i] = buf[indx+numElem+_i]; \
    count -= numElem; \
}

void *laikaM_realloc(void *buf, size_t sz);

#endif