#ifndef __BP_H__
#define __BP_H__

#include <stdbool.h>
#include <stdint.h>

void init_pred();

void branch_check_pred(uint64_t ip, bool taken);

void call_update_pred(int64_t addr);

void ret_check_pred(int64_t addr);

void bp_statistic();

#endif
