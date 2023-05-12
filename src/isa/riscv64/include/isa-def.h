#ifndef __ISA_RISCV64_H__
#define __ISA_RISCV64_H__

#include "csr.h"
#include <common.h>

typedef struct
{
    word_t gpr[32];
    vaddr_t pc;
    CSRs csr;
} riscv64_CPU_state;

// decode
typedef struct
{
    union {
        uint32_t val;
    } inst;
} riscv64_ISADecodeInfo;

#define isa_mmu_check(vaddr, len, type) (MMU_DIRECT)

#endif
