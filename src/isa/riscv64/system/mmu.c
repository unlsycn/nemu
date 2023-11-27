#include "mmu.h"

#include <isa.h>
#include <memory/paddr.h>
#include <memory/vaddr.h>

#include "common.h"

int isa_mmu_check(vaddr_t vaddr, int len, int type)
{
    uint64_t mode = cpu.csr.satp->mode;
    switch (mode)
    {
    case 0:
        return MMU_DIRECT;
    case 8:
        return MMU_TRANSLATE;
    default:
        return MMU_FAIL;
    }
}

// FIXME: the return value should be pg_paddr | MEM_RET_OK if the translation succeed
paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type)
{
    if (type == MMU_DIRECT)
        return vaddr;
    if (type == MMU_FAIL)
        return MEM_RET_FAIL;

    paddr_t base = cpu.csr.satp->ppn << PG_OFFSET;
    PageTableEntry pte;
    for (int lv = 1; lv <= 3; lv++)
    {
        paddr_t pg_paddr = base + vpn(vaddr, lv);
        pte = (PageTableEntry)paddr_read(pg_paddr, 8);
        if (!pte.v) // invalid pte
            return MEM_RET_FAIL;
        base = pte.ppn << PG_OFFSET;
    }
    return (pte.ppn << PG_OFFSET) + BITS(vaddr, PG_OFFSET - 1, 0);
}
