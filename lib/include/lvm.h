#ifndef LAIKA_VM_H
#define LAIKA_VM_H

/* Laika VM:
        This is an obfuscation technique where vital code can be executed in a
    stack-based VM, inlined into the function. The VM instruction-set is fairly
    simple, see the OP_* for avaliable opcodes and their expected arguments.
*/

#include <inttypes.h>

#include "lerror.h"

#define LAIKA_VM_CODESIZE 512
#define LAIKA_VM_STACKSIZE 64
#define LAIKA_VM_CONSTSIZE 32

struct sLaika_vm_val {
    union {
        uint8_t i;
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

#define LAIKA_MAKE_VM_IA(opcode, a) opcode, a
#define LAIKA_MAKE_VM_IAB(opcode, a, b) opcode, a, b
#define LAIKA_MAKE_VM_IABC(opcode, a, b, c) opcode, a, b, c

enum {
    OP_EXIT,
    OP_LOADCONST, /* stk_indx[uint8_t] = const_indx[uint8_t] */
    OP_READ, /* stk_indx[uint8_t] = *(int8_t*)stk_indx[uint8_t] */
    OP_WRITE, /* *(uint8_t*)stk_indx[uint8_t] = stk_indx[uint8_t] */

    /* arithmetic */
    OP_ADD, /* stk_indx[uint8_t] = stk_indx[uint8_t] + stk_indx[uint8_t] */
    OP_SUB, /* stk_indx[uint8_t] = stk_indx[uint8_t] - stk_indx[uint8_t] */
    OP_MUL, /* stk_indx[uint8_t] = stk_indx[uint8_t] * stk_indx[uint8_t] */
    OP_DIV, /* stk_indx[uint8_t] = stk_indx[uint8_t] / stk_indx[uint8_t] */
    OP_AND, /* stk_indx[uint8_t] = stk_indx[uint8_t] & stk_indx[uint8_t] */
    OP_OR,  /* stk_indx[uint8_t] = stk_indx[uint8_t] | stk_indx[uint8_t] */
    OP_XOR, /* stk_indx[uint8_t] = stk_indx[uint8_t] ^ stk_indx[uint8_t] */

    /* control-flow */
    OP_TESTJMP, /* if stk_indx[uint8_t] != 0, pc += [uint8_t] */
};

inline void laikaV_execute(struct sLaika_vm *vm) {

#define READBYTE (vm->code[vm->pc++])
#define BINOP(x) { \
    uint8_t a = READBYTE; \
    uint8_t b = READBYTE; \
    uint8_t c = READBYTE; \
    vm->stack[a] = vm->stack[b].i x vm->stack[c].i; \
    break; \
}

    while (vm->code[vm->pc]) {
        switch (vm->code[vm->pc++]) {
            case OP_LOADCONST: {
                uint8_t indx = READBYTE;
                uint8_t constIndx = READBYTE;
                vm->stack[indx] = vm->constList[constIndx];
                break; 
            }
            case OP_READ: {
                uint8_t indx = READBYTE;
                uint8_t ptr = READBYTE;
                vm->stack[indx].i = *(uint8_t*)vm->stack[ptr].ptr;
                break;
            }
            case OP_WRITE: {
                uint8_t ptr = READBYTE;
                uint8_t indx = READBYTE;
                *(uint8_t*)vm->stack[ptr].ptr = vm->stack[indx].i;
                break;
            }
            case OP_ADD: BINOP(+);
            case OP_SUB: BINOP(-);
            case OP_MUL: BINOP(*);
            case OP_DIV: BINOP(/);
            case OP_AND: BINOP(&);
            case OP_OR: BINOP(|);
            case OP_XOR: BINOP(^);
            default:
                LAIKA_ERROR("laikaV_execute: unknown opcode [%d]!", vm->code[vm->pc])
        }
    }

#undef READBYTE
#undef BINOP
}

#endif