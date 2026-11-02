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
* @file    : nc_cnn_worker_for_postprocess.c
*
* @brief   : Implementation of nc_cnn_worker_for_postprocess
*
* @author  : SoC SW team.  NextChip Inc.
*
* @date    : 2022.11.07
*
* @version : 1.0.0
********************************************************************************
* @note
* 2022.11.07 / 1.0.0 / Initial released.
*
********************************************************************************
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <mqueue.h>
#include <stdint.h>
#include <math.h>

#include "nc_utils.h"
#include "nc_cnn_aiware_runtime.h"
#include "nc_cnn_communicator.h"
#include "nc_cnn_network_includes.h"
#include "nc_cnn_worker_for_postprocess.h"
#include "nc_cnn_config_parser.h"

PtrFuncPostprc g_postprocess_and_draw_func = NULL;

static int running = 1;

void nc_cnn_postprocess_stop(void)
{
    running = 0;
}

void *nc_cnn_postprocess_task(void *arg)
{
    cnn_postprocess_arg *cnn_post_param = (cnn_postprocess_arg *)arg;
    unsigned int screen_width = cnn_post_param->target_width;
    unsigned int screen_height= cnn_post_param->target_height;

    printf("++PP TASK RUN\n");
    while (running) {
        struct inference_result_msg *npu_msg = NULL;
        // printf("WAIT PP MQ\n");
        if (nc_cnn_receive_inference_result(&npu_msg) != -1) {
            // printf("RCV PP\n");
            // Postprocess and Draw ....
            if(npu_msg->ptr_pp) {
                // printf("##coordinate fit input img size\n");
                npu_msg->ptr_pp(screen_width, screen_height, npu_msg);
            }
            else {
                printf("[WARNING] post proc func pointer is NULL\n");
            }
            nc_cnn_cleanup_inference_result(&npu_msg);
        }
    }
    nc_free_network_info();
    printf("EXIT PP_TASK!!\n");
    return NULL;
}

void nc_cnn_set_func_for_postprocess_and_cairo_draw (PtrFuncPostprc fp)
{
    g_postprocess_and_draw_func = fp;
}
