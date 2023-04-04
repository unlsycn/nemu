#include <cpu/cpu.h>
#include <difftest-def.h>
#include <isa.h>
#include <memory/paddr.h>

void difftest_memcpy(paddr_t addr, void *buf, size_t n, bool direction)
{
    if (direction == DIFFTEST_TO_REF)
    {
        memcpy(guest_to_host(addr), buf, n);
    }
    else
    {
        panic("Not implement!");
    }
}

void difftest_regcpy(void *dut, bool direction)
{
    CPU_state *ctx = (CPU_state *)dut;
    isa_difftest_regcpy(ctx, direction);
}

void difftest_exec(uint64_t n)
{
    cpu_exec(n);
}

void difftest_raise_intr(word_t NO)
{
    assert(0);
}

void init_mem();

void difftest_init(int port)
{
    /* Perform ISA dependent initialization. */
    init_mem();
    init_isa();
}
