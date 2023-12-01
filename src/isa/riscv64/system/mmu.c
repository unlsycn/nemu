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

// TODO: add more flag check
paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type)
{
    paddr_t base = cpu.csr.satp->ppn << PG_OFFSET;
    for (int lv = 1; lv <= 3; lv++)
    {
        paddr_t pg_paddr = base + vpn(vaddr, lv);
        if (lv == 3)
            return pg_paddr | MEM_RET_OK;
        PageTableEntry pte = (PageTableEntry)paddr_read(pg_paddr, 8);
        if (!pte.v || (!pte.r && pte.w))
            return MEM_RET_FAIL;

        base = pte.ppn << PG_OFFSET;
    }

    panic("Unexcepted Page Fault");
    isa_raise_intr(type == MEM_TYPE_IFETCH ? 12 : type == MEM_TYPE_READ ? 13 : 15, cpu.pc);
    return MEM_RET_FAIL;
}
