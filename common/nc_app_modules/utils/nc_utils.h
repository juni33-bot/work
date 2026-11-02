/**
********************************************************************************
* Copyright (C) 2021 NEXTCHIP Inc. All rights reserved.
* This software is the confidential and proprietary information of
* NEXTCHIP, Inc. ("Confidential Information"). You shall not disclose such
* Confidential Information and shall use it only in accordance with
* the terms of the license agreement you entered into with NEXTCHIP.
********************************************************************************
*/
/**
********************************************************************************
* @file    : nc_utils.h
*
* @brief   : util api header
*
* @author  : SoC SW team.  NextChip Inc.
*
* @date    : 2022.02.08.
*
* @version : 1.0.0
********************************************************************************
* @note
*
********************************************************************************
*/
#ifndef __UTILS_H__
#define __UTILS_H__

#include "stdint.h"
#include <net/if.h>

#define MAXVAL(a,b)             (((a) > (b)) ? (a) : (b))
#define ALIGN_UP(x, align)      (((x) + (align)-1) / (align) * (align))
#define ALIGN_DOWN(x, align)    ((x) / (align) * (align))

#define MAX_IFNAME_SIZE         (IFNAMSIZ - 1)
#define MAX_TASK_CNT            (10)

extern int nc_is_file(const char *path);
extern int nc_copy_file(const char *src, const char *dst);
extern int nc_get_file_size(char *file_path);
extern uint64_t nc_get_free_mem_size(void);
extern uint64_t nc_get_mono_time(void);
extern uint64_t nc_elapsed_time(uint64_t p_time);
extern uint64_t nc_get_mono_us_time(void);
extern uint64_t nc_elapsed_us_time(uint64_t p_time);
extern int nc_init_path_localizer(void);
extern char *nc_localize_path(const char *path);
extern int nc_get_local_IPv4(const char* if_name, char *ret_ip_addr);
extern void nc_backtrace(void);
extern void nc_fps_delay(long milliseconds);
extern uint64_t nc_get_us_from_timeval(struct timeval *ts);
extern uint32_t nc_trim(char *str);
extern unsigned int nc_read_reg(unsigned int addr);
extern int nc_write_reg(unsigned long addr, unsigned long data);
#endif  // __UTILS_H__