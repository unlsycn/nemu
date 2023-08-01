
#ifndef __ISA_LOONGARCH32R_H__
#define __ISA_LOONGARCH32R_H__

#include <common.h>

typedef struct {
  word_t gpr[32];
  vaddr_t pc;
} loongarch32r_CPU_state;

// decode
typedef struct {
  union {
    uint32_t val;
  } inst;
} loongarch32r_ISADecodeInfo;

#define isa_mmu_check(vaddr, len, type) (MMU_DIRECT)

#endif
