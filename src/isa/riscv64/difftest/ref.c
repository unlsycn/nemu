#include "../local-include/reg.h"
#include "difftest-def.h"
#include <cpu/difftest.h>
#include <isa.h>

void isa_difftest_regcpy(CPU_state *dut, bool direction)
{
    if (direction == DIFFTEST_TO_REF)
    {
        cpu.pc = dut->pc;
        for (int i = 0; i < 32; i++)
        {
            gpr(i) = dut->gpr[i];
        }
    }
    else
    {
        dut->pc = cpu.pc;
        for (int i = 0; i < 32; i++)
        {
            dut->gpr[i] = gpr(i);
        }
    }
}