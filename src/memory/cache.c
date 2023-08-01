#include "memory/cache.h"
#include "common.h"
#include "debug.h"
#include "macro.h"
#include "memory/paddr.h"
#include "utils.h"

#define exp2(x) (1 << (x))
#define mask_with_len(x) (exp2(x) - 1)

#define VADDR_LEN 32
#define WAY_WIDTH 12
#define LINE_WIDTH 4
#define WAY_COUNT 2

#define WAY_SIZE exp2(WAY_WIDTH)
#define LINE_SIZE exp2(LINE_WIDTH)
#define INDEX_WIDTH (WAY_WIDTH - LINE_WIDTH)
#define LINE_COUNT exp2(INDEX_WIDTH)
typedef struct
{
    union {
        struct
        {
            uint32_t offset : LINE_WIDTH;
            uint32_t index : INDEX_WIDTH;
            uint32_t tag : VADDR_LEN - WAY_WIDTH;
        };
        paddr_t val;
    } addr;
    bool dirty;
} cachetag_t;

typedef struct
{
    uint64_t read_cnt;
    uint64_t read_hit_cnt;
    uint64_t write_cnt;
    uint64_t write_hit_cnt;
    cachetag_t cache[LINE_COUNT][WAY_COUNT];
} cache_t;

static cache_t inst_cache;
static cache_t data_cache;

static uint64_t cache_read(cache_t *cache, paddr_t addr, int len)
{
    paddr_t base = addr & ~mask_with_len(LINE_WIDTH);
    uint32_t index = BITS(addr, WAY_WIDTH - 1, LINE_WIDTH);
    uint32_t tag = BITS(addr, 31, WAY_WIDTH);
    if (likely(in_pmem(addr)))
    {
        cache->read_cnt++;
        cachetag_t *line_ptr;
        for (int i = 0; i < WAY_COUNT; i++)
        {
            line_ptr = &cache->cache[i][index];
            if (line_ptr->addr.tag == tag)
            {
                cache->read_hit_cnt++;
                return paddr_read(addr, len);
            }
        }
        // cache miss
        line_ptr = &cache->cache[rand() % WAY_COUNT][index];
        if (line_ptr->dirty)
        {
            log_write("[ctrace] " FMT_PADDR "write back\n", line_ptr->addr.val);
            line_ptr->dirty = false;
        }
        line_ptr->addr.val = base;
    }
    return paddr_read(addr, len);
}

void cache_write(cache_t *cache, paddr_t addr, int len, word_t data)
{
    paddr_t base = addr & ~mask_with_len(LINE_WIDTH);
    uint32_t index = BITS(addr, WAY_WIDTH - 1, LINE_WIDTH);
    uint32_t tag = BITS(addr, 31, WAY_WIDTH);
    if (likely(in_pmem(base)))
    {
        cache->write_cnt++;
        cachetag_t *line_ptr;
        for (int i = 0; i < WAY_COUNT; i++)
        {
            line_ptr = &cache->cache[i][index];
            if (line_ptr->addr.tag == tag)
            {
                cache->write_hit_cnt++;
                paddr_write(addr, len, data);
                return;
            }
            // cache miss
            line_ptr = &cache->cache[rand() % WAY_COUNT][index];
            if (line_ptr->dirty)
            {
                log_write("[ctrace] " FMT_PADDR "write back\n", line_ptr->addr.val);
            }
            line_ptr->addr.val = base;
            line_ptr->dirty = true;
        }
    }
    paddr_write(addr, len, data);
}

uint64_t icache_fetch(paddr_t addr, int len)
{
    return cache_read(&inst_cache, addr, len);
}

uint64_t dcache_read(paddr_t addr, int len)
{
    return cache_read(&data_cache, addr, len);
}

void dcache_write(paddr_t addr, int len, word_t data)
{
    return cache_write(&data_cache, addr, len, data);
}

void init_cache()
{
    srand(0); // constant seed
}

void cache_statistic(void)
{
    Log("read I-Cache " NUMBERIC_FMT " times. inst fetch hit rate: %lf%%", inst_cache.read_cnt,
        (double)inst_cache.read_hit_cnt * 100 / inst_cache.read_cnt);
    Log("read D-Cache " NUMBERIC_FMT " times. cache read hit rate: %lf%%", data_cache.read_cnt,
        (double)data_cache.read_hit_cnt * 100 / data_cache.read_cnt);
    Log("write D-Cache " NUMBERIC_FMT " times. cache write hit rate: %lf%%", data_cache.write_cnt,
        (double)data_cache.write_hit_cnt * 100 / data_cache.write_cnt);
}