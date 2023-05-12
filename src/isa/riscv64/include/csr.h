#ifndef __CSR_H__
#define __CSR_H__

#include <common.h>

#define MXLEN 64
#define CSRS(f) f(mepc, 0x341) f(mtvec, 0x305) f(mstatus, 0x300) f(mcause, 0x342)

#define _WORD_T(val) word_t val;
#define CSR_TYPE(name, ...)           \
    typedef union {                   \
        struct                        \
        {                             \
            MAP(_WORD_T, __VA_ARGS__) \
        };                            \
        word_t val;                   \
    } CONCAT(name, _t);

CSR_TYPE(mepc, mepc : MXLEN)
CSR_TYPE(mtvec, base : MXLEN - 2, mode : 2)
CSR_TYPE(mstatus, SD : 1, WPRI1
         : MXLEN - 37, SXL : 2, UXL : 2, WPRI2 : 9, TSR : 1, TW : 1, TVM : 1, MXR : 1, SUM : 1, MPRV : 1, XS : 2, FS : 2,
           MPP : 2, WPRI3 : 2, SPP : 2, MPIE : 1, WPRI4 : 1, UPIE : 1, MIE : 1, WPRI5 : 1, SIE : 1, UIE : 1)
CSR_TYPE(mcause, intr : 1, code : MXLEN - 1)
#undef _WORD_T

typedef struct
{
#define CSR_PTR(name, addr) CONCAT(name, _t) * name;
    MAP(CSRS, CSR_PTR)
} CSRs;

void init_csr();
word_t csr_read(uint16_t addr);
void csr_write(uint16_t addr, word_t data);

#endif