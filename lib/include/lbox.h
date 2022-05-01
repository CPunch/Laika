#ifndef LAIKA_BOX_H
#define LAIKA_BOX_H

#include <inttypes.h>

#include "laika.h"
#include "lmem.h"
#include "lvm.h"
#include "lsodium.h"

#define LAIKA_BOX_HEAPSIZE 256

/* Laika Box: 
        Laika Boxes are obfuscated storage mediums where data is only in memory for a very short amount of time.
    Of course, this can be bypassed with a simple debugger and setting a breakpoint right after the data is 'unlocked',
    but the game of obfuscation isn't to prevent the data from being seen, it's to slow the reverse engineer down.

        2 main APIs are exposed here, laikaB_unlock() & laikaB_lock(). Both of which are inlined to make it more painful
    for the reverse engineer to quickly dump boxes from memory, forcing them to set breakpoints across the executable.
    Each box has its own VM, with it's own deobfuscation routine. This makes static analysis a painful route for string
    dumping. Some predefined boxes are made for you to use.
*/

struct sLaikaB_box {
    uint8_t unlockedData[LAIKA_BOX_HEAPSIZE];
    uint8_t code[LAIKA_VM_CODESIZE];
};

/* BOX_SKID decodes null-terminated strings using a provided xor _key. aptly named lol [SEE tools/vmtest/src/main.c] */
#define LAIKA_BOX_SKID(_key) { \
    .unlockedData = {0}, /* reserved */ \
    .code = { /* stack layout: \
            [0] - unlockedData (ptr) \
            [1] - data (ptr) \
            [2] - key (uint8_t) \
            [3] - working data (uint8_t) \
        */ \
        LAIKA_MAKE_VM_IAB(OP_LOADCONST, 0, 0), \
        LAIKA_MAKE_VM_IAB(OP_LOADCONST, 1, 1), \
        LAIKA_MAKE_VM_IAB(OP_PUSHLIT, 2, _key), \
        /* LOOP_START */ \
        LAIKA_MAKE_VM_IAB(OP_READ, 3, 1), /* load data into working data */ \
        LAIKA_MAKE_VM_IABC(OP_XOR, 3, 3, 2), /* xor data with key */ \
        LAIKA_MAKE_VM_IAB(OP_WRITE, 0, 3), /* write data to unlockedData */ \
        LAIKA_MAKE_VM_IA(OP_INCPTR, 0), \
        LAIKA_MAKE_VM_IA(OP_INCPTR, 1), \
        LAIKA_MAKE_VM_IAB(OP_TESTJMP, 3, -17), /* exit loop on null terminator */ \
        OP_EXIT \
    } \
}

LAIKA_FORCEINLINE void* laikaB_unlock(struct sLaikaB_box *box, void *data) {
    struct sLaikaV_vm vm = {
        /* boxes have 2 reserved constants, [0] for the output, [1] for the input */
        .constList = {
            LAIKA_MAKE_VM_PTR(box->unlockedData),
            LAIKA_MAKE_VM_PTR(data),
        },
        .code = {0},
        .stack = {0},
        .pc = 0
    };

    memcpy(vm.code, box->code, LAIKA_VM_CODESIZE);
    laikaV_execute(&vm);
    return (void*)box->unlockedData;
}

/* safely zeros the unlockedData using libsodium's api for clearing sensitive data from memory */
LAIKA_FORCEINLINE void* laikaB_lock(struct sLaikaB_box *box) {
    sodium_memzero(box->unlockedData, LAIKA_BOX_HEAPSIZE);
}

#endif