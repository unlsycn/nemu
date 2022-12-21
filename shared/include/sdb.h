#ifndef __SDB_H__
#define __SDB_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

    extern void init_sdb();

    extern void sdb_mainloop();

    extern void sdb_set_batch_mode();

    extern bool traval_wp();

#ifdef __cplusplus
}
#endif

#endif