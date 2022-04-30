#include <stdio.h>
#include <string.h>

#include "lvm.h"
#include "lbox.h"

/* VM BOX Demo:
    A secret message has been xor'd, this tiny bytecode chunk decodes 'data' into 
    'unlockedData'. Obviously you wouldn't want a key this simple, more obfuscation
    would be good (byte swapping, revolving xor key, etc.)
*/ 

int main(int argv, char **argc) {
    uint8_t data[] = {
        0x96, 0xBB, 0xB2, 0xB2, 0xB1, 0xFE, 0x89, 0xB1,
        0xAC, 0xB2, 0xBA, 0xFF, 0xDE, 0x20, 0xEA, 0xBA, 
        0xCE, 0xEA, 0xFC, 0x01, 0x9C, 0x23, 0x4D, 0xEE
    };

    struct sLaikaB_box box = {
        .unlockedData = {0}, /* reserved */
        .code = { /* stack layout:
                [0] - unlockedData (ptr)
                [1] - data (ptr)
                [2] - key (uint8_t)
                [3] - working data (uint8_t)
            */
            LAIKA_MAKE_VM_IAB(OP_LOADCONST, 0, 0),
            LAIKA_MAKE_VM_IAB(OP_LOADCONST, 1, 1),
            LAIKA_MAKE_VM_IAB(OP_PUSHLIT, 2, 0xDE),
            /* LOOP_START */
            LAIKA_MAKE_VM_IAB(OP_READ, 3, 1), /* load data into working data */
            LAIKA_MAKE_VM_IABC(OP_XOR, 3, 3, 2), /* xor data with key */
            LAIKA_MAKE_VM_IAB(OP_WRITE, 0, 3), /* write data to unlockedData */
            LAIKA_MAKE_VM_IA(OP_INCPTR, 0),
            LAIKA_MAKE_VM_IA(OP_INCPTR, 1),
            LAIKA_MAKE_VM_IAB(OP_TESTJMP, 3, -17), /* exit loop on null terminator */
            OP_EXIT
        }
    };

    laikaB_unlock(&box, data);
    printf("%s\n", box.unlockedData);
    laikaB_lock(&box);
    return 0;
}
