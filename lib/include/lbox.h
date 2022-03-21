#ifndef LAIKA_BOX_H
#define LAIKA_BOX_H

#include <inttypes.h>

#include "lvm.h"

/* Laika Box: 
        Laika Boxes are obfuscated storage mediums where data is only in memory for a very short amount of time.
    Of course, this can be bypassed with a simple debugger and setting a breakpoint right after the data is 'unlocked',
    but the game of obfuscation isn't to prevent the data from being seen, it's to slow the reverse engineer down.

        2 main APIs are exposed here, laikaB_unlock() & laikaB_lock(). Both of which are inlined to make it more painful
    for the reverse engineer to quickly dump boxes from memory, forcing them to set breakpoints across the executable.
    Each box has its own VM, with it's own deobfuscation routine. This makes static analysis a painful route for string
    dumping.
*/

enum {
    BOX_IP,
    BOX_PUBKEY,
    BOX_MAX
};

struct sLaikaB_box {
    uint8_t *data;
    uint8_t *unlockedData;
    sLaikaV_vm vm;
};

inline void laikaB_unlock() {

}

/* safely free's allocated buffer using libsodium's api for clearing sensitive data from memory */
inline void laikaB_lock() {

}

#endif