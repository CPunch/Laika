#ifndef LAIKA_MEM_H
#define LAIKA_MEM_H

#include "laika.h"

#define GROW_FACTOR 2

/* microsoft strikes again with their lack of support for VLAs */
#if _MSC_VER
#    define VLA(type, var, sz) type *var = laikaM_malloc(sizeof(type) * sz);
#    define ENDVLA(var)        laikaM_free(var);
#else
#    define VLA(type, var, sz) type var[sz];
#    define ENDVLA(var)        ((void)0) /* no op */
#endif

#define laikaM_malloc(sz)        laikaM_realloc(NULL, sz)
#define laikaM_free(buf)         laikaM_realloc(buf, 0)

/* ========================================[[ Vectors ]]======================================== */

#define laikaM_countVector(name) name##_COUNT
#define laikaM_capVector(name)   name##_CAP

#define laikaM_newVector(type, name)                                                               \
    type *name;                                                                                    \
    int name##_COUNT;                                                                              \
    int name##_CAP

#define laikaM_initVector(name, startCap)                                                          \
    name = NULL;                                                                                   \
    name##_COUNT = 0;                                                                              \
    name##_CAP = startCap

#define laikaM_growVector(type, name, needed)                                                      \
    if (name##_COUNT + needed >= name##_CAP || name == NULL) {                                     \
        name##_CAP = (name##_CAP + needed) * GROW_FACTOR;                                          \
        name = (type *)laikaM_realloc(name, sizeof(type) * name##_CAP);                            \
    }

/* moves vector elements above indx down by numElem, removing numElem elements at indx */
#define laikaM_rmvVector(name, indx, numElem)                                                      \
    do {                                                                                           \
        int _i, _sz = ((name##_COUNT - indx) - numElem);                                           \
        for (_i = 0; _i < _sz; _i++)                                                               \
            name[indx + _i] = name[indx + numElem + _i];                                           \
        name##_COUNT -= numElem;                                                                   \
    } while (0);

/* moves vector elements above indx up by numElem, inserting numElem elements at indx */
#define laikaM_insertVector(name, indx, numElem)                                                   \
    do {                                                                                           \
        int _i;                                                                                    \
        for (_i = name##_COUNT; _i > indx; _i--)                                                   \
            name[_i] = name[_i - 1];                                                               \
        name##_COUNT += numElem;                                                                   \
    } while (0);

void *laikaM_realloc(void *buf, size_t sz);

inline bool laikaM_isBigEndian(void)
{
    union
    {
        uint32_t i;
        uint8_t c[4];
    } _indxint = {0xDEADB33F};

    return _indxint.c[0] == 0xDE;
}

inline void laikaM_reverse(uint8_t *buf, size_t sz)
{
    int k;

    /* swap bytes, reversing the buffer */
    for (k = 0; k < (sz / 2); k++) {
        uint8_t tmp = buf[k];
        buf[k] = buf[sz - k - 1];
        buf[sz - k - 1] = tmp;
    }
}


#endif