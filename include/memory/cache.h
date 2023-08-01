#ifndef __CACHE_H__
#define __CACHE_H__

#include "common.h"

void init_cache();
uint64_t icache_fetch(paddr_t addr, int len);
uint64_t dcache_read(paddr_t addr, int len);
void dcache_write(paddr_t addr, int len, word_t data);
void cache_statistic();

#endif
