#ifndef LAIKA_VM_H
#define LAIKA_VM_H

/* Laika VM:
        This is an obfuscation technique where vital code can be executed in a
    stack-based VM, inlined into the function. The VM instruction-set is fairly
    simple, see the OP_* enum for avaliable opcodes and their expected arguments.
    The VM is turing-complete, however the instruction-set has been curated to 
    fit this specific use case.
*/

#include <inttypes.h>

#include "laika.h"
#include "lerror.h"

#define LAIKA_VM_CODESIZE 512
#define LAIKA_VM_STACKSIZE 64
#define LAIKA_VM_CONSTSIZE 32

struct sLaikaV_vm_val {
    union {
        uint8_t i;
        uint8_t *ptr;
    };
};

struct sLaikaV_vm {
    struct sLaikaV_vm_val stack[LAIKA_VM_STACKSIZE];
    struct sLaikaV_vm_val constList[LAIKA_VM_CONSTSIZE];
    uint8_t code[LAIKA_VM_CODESIZE];
    int pc;
};

#define LAIKA_MAKE_VM(_consts, _code) (struct sLaikaV_vm)({.constList = _consts, .code = _code, .pc = 0, .stack = {}})

/* constants */
#define LAIKA_MAKE_VM_INT(_i) (struct sLaikaV_vm_val)({.i = _i})
#define LAIKA_MAKE_VM_PTR(_ptr) (struct sLaikaV_vm_val)({.ptr = _ptr})
/* instructions */
#define LAIKA_MAKE_VM_IA(opcode, a) opcode, a
#define LAIKA_MAKE_VM_IAB(opcode, a, b) opcode, a, b
#define LAIKA_MAKE_VM_IABC(opcode, a, b, c) opcode, a, b, c

enum {
    OP_EXIT,
    OP_LOADCONST, /* stk_indx[uint8_t] = const_indx[uint8_t] */
    OP_PUSHLIT, /* stk_indx[uint8_t].i = uint8_t */
    OP_READ, /* stk_indx[uint8_t].i = *(int8_t*)stk_indx[uint8_t] */
    OP_WRITE, /* *(uint8_t*)stk_indx[uint8_t].ptr = stk_indx[uint8_t].i */
    OP_INCPTR, /* stk_indx[uint8_t].ptr++ */

    /* arithmetic */
    OP_ADD, /* stk_indx[uint8_t] = stk_indx[uint8_t] + stk_indx[uint8_t] */
    OP_SUB, /* stk_indx[uint8_t] = stk_indx[uint8_t] - stk_indx[uint8_t] */
    OP_MUL, /* stk_indx[uint8_t] = stk_indx[uint8_t] * stk_indx[uint8_t] */
    OP_DIV, /* stk_indx[uint8_t] = stk_indx[uint8_t] / stk_indx[uint8_t] */
    OP_AND, /* stk_indx[uint8_t] = stk_indx[uint8_t] & stk_indx[uint8_t] */
    OP_OR,  /* stk_indx[uint8_t] = stk_indx[uint8_t] | stk_indx[uint8_t] */
    OP_XOR, /* stk_indx[uint8_t] = stk_indx[uint8_t] ^ stk_indx[uint8_t] */

    /* control-flow */
    OP_TESTJMP, /* if stk_indx[uint8_t] != 0, pc += [int8_t] */

    /* misc. */
#ifdef DEBUG
    OP_DEBUG
#endif
};

LAIKA_FORCEINLINE void laikaV_execute(struct sLaikaV_vm *vm) {

#define READBYTE (vm->code[vm->pc++])
#define BINOP(x) { \
    uint8_t a = READBYTE; \
    uint8_t b = READBYTE; \
    uint8_t c = READBYTE; \
    vm->stack[a].i = vm->stack[b].i x vm->stack[c].i; \
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
            case OP_PUSHLIT: {
                uint8_t indx = READBYTE;
                uint8_t lit = READBYTE;
                vm->stack[indx].i = lit;
                break;
            }
            case OP_READ: {
                uint8_t indx = READBYTE;
                uint8_t ptr = READBYTE;
                vm->stack[indx].i = *vm->stack[ptr].ptr;
                break;
            }
            case OP_WRITE: {
                uint8_t ptr = READBYTE;
                uint8_t indx = READBYTE;
                *vm->stack[ptr].ptr = vm->stack[indx].i;
                break;
            }
            case OP_INCPTR: {
                uint8_t ptr = READBYTE;
                vm->stack[ptr].ptr++;
                break;
            }
            case OP_ADD: BINOP(+);
            case OP_SUB: BINOP(-);
            case OP_MUL: BINOP(*);
            case OP_DIV: BINOP(/);
            case OP_AND: BINOP(&);
            case OP_OR: BINOP(|);
            case OP_XOR: BINOP(^);
            case OP_TESTJMP: {
                uint8_t indx = READBYTE;
                int8_t jmp = READBYTE;

                /* if stack indx is true, jump by jmp (signed 8-bit int) */
                if (vm->stack[indx].i)
                    vm->pc += jmp;

                break;
            }
#ifdef DEBUG
            case OP_DEBUG: {
                int i;

                /* print stack info */
                for (i = 0; i < LAIKA_VM_STACKSIZE; i++)
                    printf("[%03d] - 0x%02x\n", i, vm->stack[i].i);

                break;
            }
#endif
            default:
                LAIKA_ERROR("laikaV_execute: unknown opcode [0x%02x]! pc: %d\n", vm->code[vm->pc], vm->pc);
        }
    }

#undef READBYTE
#undef BINOP
}

#endif