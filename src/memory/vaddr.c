#include <isa.h>
#include <memory/cache.h>
#include <memory/paddr.h>

#include "mmu.h"

paddr_t addr_translate(vaddr_t vaddr, int len, int type)
{
    Assert((vaddr & (len - 1)) == 0, "Misalign address = " FMT_WORD, vaddr);

    int mmu_mode = isa_mmu_check(vaddr, len, type);
    if (mmu_mode == MMU_DIRECT)
        return vaddr;
    if (mmu_mode == MMU_FAIL)
        panic("MMU Fail: Invalid satp mode");

    paddr_t pg_paddr = isa_mmu_translate(vaddr, len, type);
    if (pg_paddr & MEM_RET_FAIL)
        goto page_fault;

    PageTableEntry pte = (PageTableEntry)dcache_read(pg_paddr, 8);
    if (!pte.v || (!pte.r && pte.w)) // invalid pte
        goto page_fault;
    if (pte.u ? (cpu.priv == PRIV_S) && (!cpu.csr.sstatus->SUM || type == MEM_TYPE_IFETCH)
              : cpu.priv != PRIV_S) // wrong privilege
        goto page_fault;

    return (pte.ppn << PG_OFFSET) | BITS(vaddr, PG_OFFSET - 1, 0);

page_fault:
    panic("Page fault at vaddr = " FMT_WORD "\npte.v = %d, pte.u = %d, priv = %d, SUM = %d", vaddr, pte.v, pte.u, cpu.priv,
          cpu.csr.sstatus->SUM);
    isa_raise_intr(type == MEM_TYPE_IFETCH ? 12 : type == MEM_TYPE_READ ? 13 : 15, cpu.pc);
}

word_t vaddr_ifetch(vaddr_t addr, int len)
{
    paddr_t paddr = addr_translate(addr, len, MEM_TYPE_IFETCH);

    return icache_fetch(paddr, len);
}

word_t vaddr_read(vaddr_t addr, int len)
{
    paddr_t paddr = addr_translate(addr, len, MEM_TYPE_READ);

    word_t data = dcache_read(paddr, len);

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

    dcache_write(paddr, len, data);
}
