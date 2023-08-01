#ifndef __DIFFTEST_DEF_H__
#define __DIFFTEST_DEF_H__

#include <generated/autoconf.h>
#include <macro.h>
#include <stdint.h>

#define __EXPORT __attribute__((visibility("default")))
enum
{
    DIFFTEST_TO_DUT,
    DIFFTEST_TO_REF
};

#if defined(CONFIG_ISA_x86)
#define DIFFTEST_REG_SIZE (sizeof(uint32_t) * 9) // GPRs + pc
#elif defined(CONFIG_ISA_mips32)
#define DIFFTEST_REG_SIZE (sizeof(uint32_t) * 38) // GPRs + status + lo + hi + badvaddr + cause + pc
#elif defined(CONFIG_ISA_riscv)
#define RISCV_GPR_TYPE MUXDEF(CONFIG_RV64, uint64_t, uint32_t)
#define RISCV_GPR_NUM MUXDEF(CONFIG_RVE, 16, 32)
#define DIFFTEST_REG_SIZE (sizeof(RISCV_GPR_TYPE) * (RISCV_GPR_NUM + 1)) // GPRs + pc
#elif defined(CONFIG_ISA_loongarch32r)
#define DIFFTEST_REG_SIZE (sizeof(uint32_t) * 33) // GPRs + pc
#else
#error Unsupport ISA
#endif

#endif
