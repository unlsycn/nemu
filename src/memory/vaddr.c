#include <isa.h>
#include <memory/cache.h>
#include <memory/paddr.h>

#include "mmu.h"

paddr_t addr_translate(vaddr_t vaddr, int len, int type)
{
    int mmu_mode = isa_mmu_check(vaddr, len, type);
    if (mmu_mode == MMU_DIRECT)
        return vaddr;
    if (mmu_mode == MMU_FAIL)
        panic("MMU Fail: Invalid satp mode");

    paddr_t pg_paddr = isa_mmu_translate(vaddr, len, type);
    Assert(!(pg_paddr & MEM_RET_FAIL), "Page fault at vaddr = %lx", vaddr);
    PageTableEntry pte = (PageTableEntry)paddr_read(pg_paddr, 8);

    return (pte.ppn << PG_OFFSET) + BITS(vaddr, PG_OFFSET - 1, 0);
}

word_t vaddr_ifetch(vaddr_t addr, int len)
{
    paddr_t paddr = addr_translate(addr, len, MEM_TYPE_IFETCH);

    return MUXDEF(CONFIG_CACHE_SIM, icache_fetch(paddr, len), paddr_read(paddr, len));
}

word_t vaddr_read(vaddr_t addr, int len)
{
    paddr_t paddr = addr_translate(addr, len, MEM_TYPE_READ);

    word_t data = MUXDEF(CONFIG_CACHE_SIM, dcache_read(paddr, len), paddr_read(paddr, len));

#ifdef CONFIG_MTRACE
    if (paddr >= CONFIG_MTRACE_LEFT && paddr <= CONFIG_MTRACE_RIGHT)
        log_write("[mtrace] vaddr = " FMT_WORD ", paddr = " FMT_PADDR " is READ %d bytes at pc = " FMT_WORD ", data = " FMT_WORD
                  "\n",
                  addr, paddr, len, cpu.pc, data);
#endif
    return data;
}

void vaddr_write(vaddr_t addr, int len, word_t data)
{
    paddr_t paddr = addr_translate(addr, len, MEM_TYPE_WRITE);

#ifdef CONFIG_MTRACE
    if (paddr >= CONFIG_MTRACE_LEFT && paddr <= CONFIG_MTRACE_RIGHT)
        log_write("[mtrace] vaddr = " FMT_WORD ", paddr = " FMT_PADDR " is WRITTEN %d bytes at pc = " FMT_WORD
                  ", data = " FMT_WORD "\n",
                  addr, paddr, len, cpu.pc, data);
#endif

    MUXDEF(CONFIG_CACHE_SIM, dcache_write(paddr, len, data), paddr_write(paddr, len, data));
}
