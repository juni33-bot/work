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
* @file    : nc_streamer.h
*
* @brief   : nc_streamer header
*
* @author  : SoC SW team.  NextChip Inc.
*
* @date    : 2023.07.21
*
* @version : 1.0.0
********************************************************************************
* @note
* 2023.07.21 / 1.0.0 / Initial released.
*
********************************************************************************
*/
#ifndef __NC_STREAM_SERVER_H__
#define __NC_STREAM_SERVER_H__

#define STREAM_PORT     (13001)

extern void nc_start_stream_server(int port);
extern void nc_shutdown_stream_server(void);
extern int nc_push_stream_data(unsigned char *ptr_send_data, int send_data_size, int with_header, unsigned char *ptr_header, int header_size);

#endif // __NC_STREAM_SERVER_H__