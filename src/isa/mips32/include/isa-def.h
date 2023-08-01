
#ifndef __ISA_MIPS32_H__
#define __ISA_MIPS32_H__

#include <common.h>

typedef struct {
  word_t gpr[32];
  word_t pad[5];
  vaddr_t pc;
} mips32_CPU_state;

// decode
typedef struct {
  union {
    uint32_t val;
  } inst;
} mips32_ISADecodeInfo;

#define isa_mmu_check(vaddr, len, type) (MMU_DIRECT)

#endif
