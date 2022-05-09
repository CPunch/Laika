#include <stdio.h>
#include <string.h>

#include "lvm.h"
#include "lbox.h"

/* VM BOX Demo:
    A secret message has been xor'd, the BOX_SKID is used to decode the message.
*/ 

#define VMTEST_STR_DATA { \
        0x96, 0xBB, 0xB2, 0xB2, 0xB1, 0xFE, 0x89, 0xB1, \
        0xAC, 0xB2, 0xBA, 0xFF, 0xDE, 0x20, 0xEA, 0xBA, /* you can see the key here, 0xDE ^ 0xDE is the NULL terminator lol */ \
        0xCE, 0xEA, 0xFC, 0x01, 0x9C, 0x23, 0x4D, 0xEE \
    };

int main(int argv, char **argc) {
    LAIKA_BOX_STARTVAR(char*, str, LAIKA_BOX_SKID(0xDE), VMTEST_STR_DATA)
    printf("%s\n", str);
    LAIKA_BOX_ENDVAR(str)
    return 0;
}
