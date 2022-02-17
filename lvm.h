#ifndef LAIKA_VM_H
#define LAIKA_VM_H

/* Laika VM:
        This is an obfuscation technique where vital code can be executed in a
    stack-based VM, inlined into the function. The VM instruction-set is fairly
    simple, see the OP_* for avaliable opcodes and their expected arguments.
*/

#define LAIKA_VM_CODESIZE 512
#define LAIKA_VM_STACKSIZE 64
#define LAIKA_VM_CONSTSIZE 32

struct sLaika_vm_val {
    union {
        int i;
        void *ptr;
    };
};

struct sLaika_vm {
    struct sLaika_vm_val stack[LAIKA_VM_STACKSIZE];
    struct sLaika_vm_val constList[LAIKA_VM_CONSTSIZE];
    uint8_t code[LAIKA_VM_CODESIZE];
    int pc;
};

#define LAIKA_MAKE_VM_INT(i) (struct sLaika_vm_val)({.i = i})
#define LAIKA_MAKE_VM_PTR(ptr) (struct sLaika_vm_val)({.ptr = ptr})
#define LAIKA_MAKE_VM(consts, code) (struct sLaika_vm)({.constList = consts, .code = code, .pc = 0})

enum {
    OP_EXIT,
    OP_LOADCONST, /* stk_indx[uint8_t] = const_indx[uint8_t] */
    OP_LOAD,

    /* arithmetic */
    OP_ADD, /* stk_indx[uint8_t] = stk_indx[uint8_t] + stk_indx[uint8_t] */
    OP_SUB, /* stk_indx[uint8_t] = stk_indx[uint8_t] - stk_indx[uint8_t] */
    OP_MUL, /* stk_indx[uint8_t] = stk_indx[uint8_t] * stk_indx[uint8_t] */
    OP_DIV, /* stk_indx[uint8_t] = stk_indx[uint8_t] / stk_indx[uint8_t] */
    OP_AND, /* stk_indx[uint8_t] = stk_indx[uint8_t] & stk_indx[uint8_t] */
    OP_OR,  /* stk_indx[uint8_t] = stk_indx[uint8_t] | stk_indx[uint8_t] */
    OP_XOR, /* stk_indx[uint8_t] = stk_indx[uint8_t] ^ stk_indx[uint8_t] */

    /* control-flow */
    OP_TESTJMP, /* if stk_indx[uint8_t] != 0, pc = [uint8_t] */
};

#endif