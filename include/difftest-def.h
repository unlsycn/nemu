#ifndef __DIFFTEST_DEF_H__
#define __DIFFTEST_DEF_H__

#include <generated/autoconf.h>
#include <stdint.h>

enum
{
    DIFFTEST_TO_DUT,
    DIFFTEST_TO_REF
};

#if defined(CONFIG_ISA_x86)
#define DIFFTEST_REG_SIZE (sizeof(uint32_t) * 9) // GPRs + pc
#elif defined(CONFIG_ISA_mips32)
#define DIFFTEST_REG_SIZE (sizeof(uint32_t) * 38) // GPRs + status + lo + hi + badvaddr + cause + pc
#elif defined(CONFIG_ISA_riscv32)
#define DIFFTEST_REG_SIZE (sizeof(uint32_t) * 33) // GPRs + pc
#elif defined(CONFIG_ISA_riscv64)
#define DIFFTEST_REG_SIZE (sizeof(uint64_t) * 33) // GPRs + pc
#elif defined(CONFIG_ISA_loongarch32r)
#define DIFFTEST_REG_SIZE (sizeof(uint32_t) * 33) // GPRs + pc
#else
#error Unsupport ISA
#endif

#endif
