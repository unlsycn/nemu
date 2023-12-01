#ifndef __ISA_RISCV64_H__
#define __ISA_RISCV64_H__

#include <common.h>

#include "csr.h"

typedef enum
{
    PRIV_U,
    PRIV_S,
    PRIV_RESERVED,
    PRIV_M
} priv_t;

typedef struct
{
    word_t gpr[32];
    vaddr_t pc;
    CSRs csr;
    priv_t priv;
} riscv64_CPU_state;

// decode
typedef struct
{
    union {
        uint32_t val;
    } inst;
} riscv64_ISADecodeInfo;

#endif
