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
* @file    : nc_cnn_communicator.c
*
* @brief   : nc_cnn_communicator source code
*
* @author  : SoC SW team.  NextChip Inc.
*
* @date    : 2022.11.10
*
* @version : 1.0.0
********************************************************************************
* @note
* 2022.11.10 / 1.0.0 / Initial released.
*
********************************************************************************
*/

#include "nc_cnn_communicator.h"

int nc_cnn_send_inference_result(struct cnn_config* config, uint64_t time_stamp, cnn_output_info cnn_output)
{
    // send message to postprocess task
    struct inference_result_msg *npu_msg;
    int ret = 0;

    struct mq_attr attr;
    attr.mq_maxmsg = MAX_MQ_MSG_CNT;
    attr.mq_msgsize = sizeof(struct inference_result_msg*);
    int oflag = O_WRONLY | O_CREAT;
    // uint64_t s_time = nc_get_mono_us_time();
    mqd_t mfd = mq_open(MQ_NAME_DATA, oflag, 0666, &attr);
    // printf("......... [%s] mq_open() elapsed : %llu us\n", __FUNCTION__, nc_elapsed_us_time(s_time));
    if (mfd == -1) {
        perror("mq open error");
        return -1;
    }

    npu_msg = (struct inference_result_msg*)malloc(sizeof(struct inference_result_msg));
    npu_msg->network_id = config->network_id;
    npu_msg->time_stamp = time_stamp;
    npu_msg->cnn_output = cnn_output;
    npu_msg->network_id = cnn_output.net_id;
    npu_msg->ptr_pp = config->callback_pp;
    if ((ret = mq_send(mfd, (const char *)&npu_msg, attr.mq_msgsize, 1)) == -1) {
        printf("errno of mq_send = %d\n", errno);
    }
    mq_close(mfd);
    return 0;
}

int nc_cnn_receive_inference_result (inference_result_msg **out_inference_result)
{
    int ret = 0;

    struct mq_attr attr;
    attr.mq_maxmsg = MAX_MQ_MSG_CNT;
    attr.mq_msgsize = sizeof(struct inference_result_msg *);
    int oflag = O_RDONLY | O_CREAT;
    // uint64_t s_time = nc_get_mono_us_time();
    mqd_t mfd = mq_open(MQ_NAME_DATA, oflag, 0666, &attr);
    // printf("......... [%s] mq_open() elapsed : %llu us\n", __FUNCTION__, nc_elapsed_us_time(s_time));
    if (mfd == -1) {
        perror("mq open error");
        return -1;
    }

    if ((ret = (int32_t)mq_receive(mfd, (char*)out_inference_result, attr.mq_msgsize, NULL)) == -1) {
        printf("errno of mq_receive = %d\n", errno);
    }
    mq_close(mfd);
    return ret;
}

void nc_cnn_cleanup_inference_result (inference_result_msg **out_inference_result)
{
    if (*out_inference_result) {
        free((*out_inference_result)->cnn_output.tinfo);
        free((*out_inference_result)->cnn_output.tiled_data);
        free((*out_inference_result));
    }
}