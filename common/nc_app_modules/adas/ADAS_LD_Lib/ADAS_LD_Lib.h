/**
********************************************************************************
* Copyright (C) 2024 NEXTCHIP Inc. All rights reserved.
* This software is the confidential and proprietary information of
* NEXTCHIP, Inc. ("Confidential Information"). You shall not disclose such
* Confidential Information and shall use it only in accordance with
* the terms of the license agreement you entered into with NEXTCHIP.
********************************************************************************
*/
/**
********************************************************************************
* @file    : ADAS_LD_Lib.h
*
* @brief   : ADAS LD library header
*
* @author  : AI SW team.  NextChip Inc.
*
* @date    : 2024.
*
* @version : 1.0.0
********************************************************************************
* @note
* 2024. / 1.0.0 / Initial released.
*
********************************************************************************
*/

#ifndef __ADAS_LD_LIB_H__
#define __ADAS_LD_LIB_H__


extern int NC_ADAS_OPEN(void);
extern int NC_ADAS_CLOSE(void);

extern void *ld_task(void *arg);

extern void draw_ld_output_opengl(void);
extern void ld_opengl_program_set(void *prog);

#endif // __ADAS_LD_LIB_H__