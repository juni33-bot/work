/**
********************************************************************************
* Copyright (C) 2022 NEXTCHIP Inc. All rights reserved.
* This software is the confidential and proprietary information of 
* NEXTCHIP, Inc. ("Confidential Information"). You shall not disclose such
* Confidential Information and shall use it only in accordance with 
* the terms of the license agreement you entered into with NEXTCHIP.
********************************************************************************
*/
/**
********************************************************************************
* @file    : nc_logo.h 
*
* @brief   : NextChip log image
*
* @author  : SoC SW team.  NextChip Inc.  
*
* @date    : 2022.09.06
*
* @version : 1.0.0
********************************************************************************
* @note
* 2022.09.06 / 1.0.0 / Initial released.
* 
********************************************************************************
*/ 

#ifndef __NC_LOGO_H__
#define __NC_LOGO_H__

#define NC_LOGO_W   (329)
#define NC_LOGO_H   (83)

#include "cairo.h"

extern unsigned char nc_logo_data[NC_LOGO_H][NC_LOGO_W*4];
extern void nc_draw_nextchip_logo(cairo_t* cr, int pos_x, int pos_y);

#endif // __NC_LOGO_H__
