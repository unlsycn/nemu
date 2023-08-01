#include "memory/cache.h"
#include "common.h"
#include "debug.h"
#include "macro.h"
#include "memory/host.h"
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
#define mask mask_with_len(LINE_WIDTH);

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
    bool valid;
    bool dirty;
    uint8_t data[LINE_SIZE];
} cacheline_t;

typedef struct
{
    uint64_t read_cnt;
    uint64_t read_hit_cnt;
    uint64_t write_cnt;
    uint64_t write_hit_cnt;
    cacheline_t cache[WAY_COUNT][LINE_COUNT];
} cache_t;

static cache_t inst_cache;
static cache_t data_cache;

static void cacheline_read(cacheline_t *line_ptr, paddr_t base)
{
    log_write("[ctrace] cacheline read memory from address = " FMT_PADDR "\n", base);
    for (int i = 0; i < LINE_SIZE; i += 8)
        host_write(&line_ptr->data[i], 8, paddr_read(base + i, 8)); // write mem(base) to cacheline
    line_ptr->addr.val = base;
    line_ptr->valid = true;
    line_ptr->dirty = false;
}

static void cacheline_write(cacheline_t *line_ptr)
{
    if (line_ptr->dirty)
    {
        log_write("[ctrace] cacheline = " FMT_PADDR " write back to memory\n", line_ptr->addr.val);
        for (int i = 0; i < LINE_SIZE; i += 8)
        {
            paddr_write(line_ptr->addr.val + i, 8, host_read(&line_ptr->data[i], 8)); // write cacheline back to mem
        }
    }
}

static uint64_t read_cachedata(cacheline_t *line_ptr, int offset, int len)
{
    return host_read(&line_ptr->data[offset], len);
}

static void write_cachedata(cacheline_t *line_ptr, int offset, int len, word_t data)
{
    host_write(&line_ptr->data[offset], len, data);
    line_ptr->dirty = true;
}

static uint64_t cache_read(cache_t *cache, paddr_t addr, int len)
{
    uint32_t offset = addr & mask;
    paddr_t base = addr & ~mask;
    uint32_t index = BITS(addr, WAY_WIDTH - 1, LINE_WIDTH);
    uint32_t tag = BITS(addr, 31, WAY_WIDTH);
    cache->read_cnt++;
    cacheline_t *line_ptr;
    for (int i = 0; i < WAY_COUNT; i++)
    {
        line_ptr = &cache->cache[i][index];
        if (line_ptr->addr.tag == tag && line_ptr->valid)
        {
            cache->read_hit_cnt++;
            return read_cachedata(line_ptr, offset, len);
        }
    }
    // cache miss
    line_ptr = &cache->cache[rand() % WAY_COUNT][index];
    cacheline_write(line_ptr);
    cacheline_read(line_ptr, base);
    return read_cachedata(line_ptr, offset, len);
}

void cache_write(cache_t *cache, paddr_t addr, int len, word_t data)
{
    uint32_t offset = addr & mask;
    paddr_t base = addr & ~mask;
    uint32_t index = BITS(addr, WAY_WIDTH - 1, LINE_WIDTH);
    uint32_t tag = BITS(addr, 31, WAY_WIDTH);
    cache->write_cnt++;
    cacheline_t *line_ptr;
    for (int i = 0; i < WAY_COUNT; i++)
    {
        line_ptr = &cache->cache[i][index];
        if (line_ptr->addr.tag == tag && line_ptr->valid)
        {
            cache->write_hit_cnt++;
            write_cachedata(line_ptr, offset, len, data);
            return;
        }
    }
    // cache miss
    line_ptr = &cache->cache[rand() % WAY_COUNT][index];
    cacheline_write(line_ptr);
    cacheline_read(line_ptr, base);
    write_cachedata(line_ptr, offset, len, data);
    return;
}

uint64_t icache_fetch(paddr_t addr, int len)
{
    return cache_read(&inst_cache, addr, len);
}

uint64_t dcache_read(paddr_t addr, int len)
{
    if (likely(in_pmem(addr)))
    {
        IFDEF(CONFIG_CTRACE, log_write("[ctrace] READ D-Cache with address = " FMT_PADDR "\n", addr));
        return cache_read(&data_cache, addr, len);
    }
    return paddr_read(addr, len);
}

void dcache_write(paddr_t addr, int len, word_t data)
{
    if (likely(in_pmem(addr)))
    {
        IFDEF(CONFIG_CTRACE,
              log_write("[ctrace] WRITE D-Cache with address = " FMT_PADDR ", data = " FMT_WORD_LH "\n", addr, data));
        cache_write(&data_cache, addr, len, data);
        return;
    }
    paddr_write(addr, len, data);
}

void flush_cache()
{
    log_write("[ctrace] cache flushed\n");
    for (int way = 0; way < WAY_COUNT; way++)
    {
        for (int line = 0; line < LINE_COUNT; line++)
        {
            inst_cache.cache[way][line].valid = false;
            cacheline_write(&data_cache.cache[way][line]);
        }
    }
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