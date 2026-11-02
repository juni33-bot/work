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
* @file    : nc_gpio.h 
*
* @brief   : nc_gpio api header
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

#include <stdint.h>

#ifndef __GPIO_H__
#define __GPIO_H__

#define GPIO_IN 0
#define GPIO_OUT 1

#define GPIO_HIGH   1
#define GPIO_LOW   0

#define GPIOA 0
#define GPIOB 32
#define GPIOC 64
#define GPIOD 96
#define GPIOE 128

extern int nc_gpio_config(uint32_t port, uint32_t pin, uint32_t dir);
extern uint32_t nc_gpio_get(uint32_t port, uint32_t pin);
extern int nc_gpio_set(uint32_t port, uint32_t pin);
extern int nc_gpio_clr(uint32_t port, uint32_t pin);

#endif  // __GPIO_H__