#include "common.h"
#include "macro.h"
#include <isa.h>
#include <memory/cache.h>
#include <memory/paddr.h>

word_t vaddr_ifetch(vaddr_t addr, int len)
{
    return MUXDEF(CONFIG_CACHE_SIM, icache_fetch(addr, len), paddr_read(addr, len));
}

word_t vaddr_read(vaddr_t addr, int len)
{
    return MUXDEF(CONFIG_CACHE_SIM, dcache_read(addr, len), paddr_read(addr, len));
}

void vaddr_write(vaddr_t addr, int len, word_t data)
{
    MUXDEF(CONFIG_CACHE_SIM, dcache_write(addr, len, data), paddr_write(addr, len, data));
}
