#include "../local-include/reg.h"
#include <cpu/difftest.h>
#include <isa.h>

extern const char *regs[];

bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc)
{
    ref_difftest_regcpy(ref_r, DIFFTEST_TO_DUT);
    if (!difftest_check_reg("pc", pc, ref_r->pc, cpu.pc))
        return false;
    for (int i = 0; i < 32; i++)
    {
        if (!difftest_check_reg(regs[i], pc, ref_r->gpr[i], gpr(i)))
            return false;
    }
    return true;
}

void isa_difftest_attach()
{
}
