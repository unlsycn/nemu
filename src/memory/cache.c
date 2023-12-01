#include "memory/cache.h"
#include "common.h"
#include "debug.h"
#include "macro.h"
#include "memory/host.h"
#include "memory/paddr.h"
#include "utils.h"

#define exp2(x) (1 << (x))
#define mask_with_len(x) (exp2(x) - 1)

#define WAY_WIDTH 12
#define LINE_WIDTH 4
#define WAY_COUNT 2
#define VICTIM_COUNT 8

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
            uint32_t tag : PADDR_LEN - WAY_WIDTH;
        };
        struct
        {
            uint32_t _offset : LINE_WIDTH;
            uint32_t victim_tag : PADDR_LEN - LINE_WIDTH;
        };
        paddr_t val;
    } addr;
    bool valid;
    bool dirty;
    uint8_t data[LINE_SIZE];
} cacheline_t;

typedef struct
{
    struct
    {
        uint64_t read_cnt;
        uint64_t read_hit_cnt;
        uint64_t victim_read_hit_cnt;
        uint64_t write_cnt;
        uint64_t write_hit_cnt;
        uint64_t victim_write_hit_cnt;
        uint64_t writeback_cnt;
    } stat;
    cacheline_t cache[WAY_COUNT][LINE_COUNT];
    uint32_t victim_lru;
    cacheline_t victim[VICTIM_COUNT];
} cache_t;

static cache_t inst_cache;
static cache_t data_cache;

static void cacheline_read(cacheline_t *line_ptr, paddr_t base)
{
    IFDEF(CONFIG_CTRACE, log_write("[ctrace] cacheline read memory from address = " FMT_PADDR "\n", base));
    for (int i = 0; i < LINE_SIZE; i += 8)
        host_write(&line_ptr->data[i], 8, paddr_read(base + i, 8)); // write mem(base) to cacheline
    line_ptr->addr.val = base;
    line_ptr->valid = true;
    line_ptr->dirty = false;
}

static bool cacheline_write(cacheline_t *line_ptr)
{
    if (line_ptr->dirty)
    {
        IFDEF(CONFIG_CTRACE, log_write("[ctrace] cacheline = " FMT_PADDR " write back to memory\n", line_ptr->addr.val));
        for (int i = 0; i < LINE_SIZE; i += 8)
        {
            paddr_write(line_ptr->addr.val + i, 8, host_read(&line_ptr->data[i], 8)); // write cacheline back to mem
        }
    }
    return line_ptr->dirty;
}

static int swap_victim(cacheline_t *line_ptr, cacheline_t *victim_ptr, int lru)
{
    const int size = sizeof(cacheline_t);
    uint8_t temp[size];
    memcpy(temp, victim_ptr, size);
    memcpy(victim_ptr, line_ptr, size);
    memcpy(line_ptr, temp, size);
    return lru + 1 >= VICTIM_COUNT ? 0 : lru + 1;
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

static word_t cache_access(cache_t *cache, paddr_t addr, int len, bool is_write, word_t data)
{
    if (!is_write)
        Assert(data == 0, "cache read with non-zero data");
    uint32_t offset = addr & mask;
    paddr_t base = addr & ~mask;
    uint32_t index = BITS(addr, WAY_WIDTH - 1, LINE_WIDTH);
    uint32_t tag = BITS(addr, 31, WAY_WIDTH);
    uint32_t victim_tag = BITS(addr, 31, LINE_WIDTH);

    if (is_write)
        cache->stat.write_cnt++;
    else
        cache->stat.read_cnt++;
    cacheline_t *line_ptr;
    cacheline_t *victim_ptr;
    for (int i = 0; i < WAY_COUNT; i++)
    {
        line_ptr = &cache->cache[i][index];
        if (line_ptr->addr.tag == tag && line_ptr->valid)
        {
            if (is_write)
            {
                cache->stat.write_hit_cnt++;
                write_cachedata(line_ptr, offset, len, data);
                return 0;
            }
            else
            {
                cache->stat.read_hit_cnt++;
                return read_cachedata(line_ptr, offset, len);
            }
        }
    }
#if VICTIM_COUNT != 0
    // check victim cache
    line_ptr = &cache->cache[rand() % WAY_COUNT][index];
    for (int i = 0; i < VICTIM_COUNT; i++)
    {
        victim_ptr = &cache->victim[i];
        if (victim_ptr->addr.victim_tag == victim_tag && victim_ptr->valid)
        {
            if (is_write)
            {
                cache->stat.victim_write_hit_cnt++;
                cache->victim_lru = swap_victim(line_ptr, victim_ptr, cache->victim_lru);
                write_cachedata(line_ptr, offset, len, data);
                return 0;
            }
            else
            {
                cache->stat.victim_read_hit_cnt++;
                cache->victim_lru = swap_victim(line_ptr, victim_ptr, cache->victim_lru);
                return read_cachedata(line_ptr, offset, len);
            }
        }
    }
    // cache miss
    victim_ptr = &cache->victim[cache->victim_lru];
    cache->stat.writeback_cnt += cacheline_write(victim_ptr);
    cache->victim_lru = swap_victim(line_ptr, victim_ptr, cache->victim_lru);
#else
    cache->stat.writeback_cnt += cacheline_write(line_ptr);
#endif
    cacheline_read(line_ptr, base);
    if (is_write)
    {
        write_cachedata(line_ptr, offset, len, data);
        return 0;
    }
    else
    {
        return read_cachedata(line_ptr, offset, len);
    }
}

word_t icache_fetch(paddr_t addr, int len)
{
    return cache_access(&inst_cache, addr, len, false, 0);
}

word_t dcache_read(paddr_t addr, int len)
{
    if (likely(in_pmem(addr)))
    {
        IFDEF(CONFIG_CTRACE, log_write("[ctrace] READ D-Cache with address = " FMT_PADDR "\n", addr));
        return cache_access(&data_cache, addr, len, false, 0);
    }
    return paddr_read(addr, len);
}

void dcache_write(paddr_t addr, int len, word_t data)
{
    if (likely(in_pmem(addr)))
    {
        IFDEF(CONFIG_CTRACE,
              log_write("[ctrace] WRITE D-Cache with address = " FMT_PADDR ", data = " FMT_WORD_LH "\n", addr, data));
        Assert(cache_access(&data_cache, addr, len, true, data) == 0, "cache write with non-zero return value");
        return;
    }
    paddr_write(addr, len, data);
}

void flush_cache()
{
    IFNDEF(CONFIG_CACHE_SIM, return);
    IFDEF(CONFIG_CTRACE, log_write("[ctrace] cache flushed\n"));
    for (int way = 0; way < WAY_COUNT; way++)
    {
        for (int line = 0; line < LINE_COUNT; line++)
        {
            inst_cache.cache[way][line].valid = false;
            cacheline_write(&data_cache.cache[way][line]);
        }
        for (int i = 0; i < VICTIM_COUNT; i++)
        {
            inst_cache.victim[i].valid = false;
            cacheline_write(&data_cache.victim[i]);
        }
    }
}

void init_cache()
{
    srand(0); // constant seed
}

static inline double hit_rate(cache_t *cache, bool is_write)
{
    if (is_write)
        return ((double)cache->stat.write_hit_cnt + cache->stat.victim_write_hit_cnt) * 100 / cache->stat.write_cnt;
    return ((double)cache->stat.read_hit_cnt + cache->stat.victim_read_hit_cnt) * 100 / cache->stat.read_cnt;
}

static inline double victim_hit_rate(cache_t *cache, bool is_write)
{
    if (is_write)
        return (double)cache->stat.victim_write_hit_cnt * 100 / cache->stat.write_cnt;
    return (double)cache->stat.victim_read_hit_cnt * 100 / cache->stat.read_cnt;
}

static inline double dirty_rate(cache_t *cache)
{
    return (double)cache->stat.writeback_cnt * 100 /
           (cache->stat.read_cnt + cache->stat.write_cnt - cache->stat.read_hit_cnt - cache->stat.write_hit_cnt -
            cache->stat.victim_read_hit_cnt - cache->stat.victim_write_hit_cnt);
}

void cache_statistic(void)
{
    Log("%d-way cache with %d lines which each size = %d Bytes, victim cache count = %d", WAY_COUNT, exp2(INDEX_WIDTH),
        LINE_SIZE, VICTIM_COUNT);
    Log("read I-Cache " NUMBERIC_FMT " times. inst fetch hit rate = %lf%% with victim hit %lf%%", inst_cache.stat.read_cnt,
        hit_rate(&inst_cache, false), victim_hit_rate(&inst_cache, false));
    Log("read D-Cache " NUMBERIC_FMT " times. cache read hit rate = %lf%% with victim hit %lf%%", data_cache.stat.read_cnt,
        hit_rate(&data_cache, false), victim_hit_rate(&data_cache, false));
    Log("write D-Cache " NUMBERIC_FMT " times. cache write hit rate = %lf%% with victim hit %lf%%", data_cache.stat.write_cnt,
        hit_rate(&data_cache, true), victim_hit_rate(&data_cache, true));
    Log("dirty rate of missing line in D-Cache = %lf%%", dirty_rate(&data_cache));
}