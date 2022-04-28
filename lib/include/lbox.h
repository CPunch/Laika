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
    dumping.
*/

struct sLaikaB_box {
    uint8_t unlockedData[LAIKA_BOX_HEAPSIZE];
    uint8_t code[LAIKA_VM_CODESIZE];
};

LAIKA_FORCEINLINE void* laikaB_unlock(struct sLaikaB_box *box, void *data) {
    struct sLaikaV_vm vm = {.pc = 0};
    memcpy(vm.code, box->code, LAIKA_VM_CODESIZE);

    /* boxes have 2 reserved constants, 0 for the output, 1 for the input */
    vm.constList[0].ptr = box->unlockedData;
    vm.constList[1].ptr = data;

    laikaV_execute(&vm);
    return (void*)box->unlockedData;
}

/* safely zeros the unlockedData using libsodium's api for clearing sensitive data from memory */
LAIKA_FORCEINLINE void* laikaB_lock(struct sLaikaB_box *box) {
    sodium_memzero(box->unlockedData, LAIKA_BOX_HEAPSIZE);
}

#endif