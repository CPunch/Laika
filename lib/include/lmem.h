#ifndef LAIKA_MEM_H
#define LAIKA_MEM_H

#include "laika.h"

#define GROW_FACTOR 2

/* microsoft strikes again with their lack of support for VLAs */
#if _MSC_VER
#define VLA(type, var, sz) type var = laikaM_malloc(sizeof(type)*sz);
#define ENDVLA(var) laikaM_free(var); 
#else
#define VLA(type, var, sz) type var[sz];
/* stubbed */
#define ENDVLA(var)
#endif

#define laikaM_malloc(sz) laikaM_realloc(NULL, sz)
#define laikaM_free(buf) laikaM_realloc(buf, 0)

#define laikaM_growarray(type, buf, needed, count, capacity) \
    if (count + needed >= capacity || buf == NULL) { \
        capacity = (capacity + needed) * GROW_FACTOR; \
        buf = (type*)laikaM_realloc(buf, sizeof(type)*capacity); \
    }

/* moves array elements above indx down by numElem, removing numElem elements at indx */ 
#define laikaM_rmvarray(buf, count, indx, numElem) { \
    int _i, _sz = ((count-indx)-numElem); \
    for (_i = 0; _i < _sz; _i++) \
        buf[indx+_i] = buf[indx+numElem+_i]; \
    count -= numElem; \
}

/* moves array elements above indx up by numElem, inserting numElem elements at indx */ 
#define laikaM_insertarray(buf, count, indx, numElem) { \
    int _i; \
    for (_i = count; _i > indx; _i--) \
        buf[_i] = buf[_i-1]; \
    count += numElem; \
}

void *laikaM_realloc(void *buf, size_t sz);

#endif