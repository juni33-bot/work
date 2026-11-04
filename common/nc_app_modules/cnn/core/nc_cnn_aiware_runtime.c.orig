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
* @file    : nc_cnn_aiware_runtime.c
*
* @brief   : Implementation of nc_cnn_aiware_runtime
*
* @author  : SoC SW team.  NextChip Inc.
*
* @date    : 2021.10.08.
*
* @version : 1.0.0
********************************************************************************
* @note
* 10.08.2021 / 1.0.0 / Initial released.
*
********************************************************************************
*/
/*
********************************************************************************
*               INCLUDES
********************************************************************************
*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#include <sys/time.h>
#include <mqueue.h>
#include <stdlib.h>
#include <string.h>
#include <linux/types.h>
#include <linux/ioctl.h>
#include <errno.h>
#include <stdint.h>
#include "nc_utils.h"
#include "nc_neon.h"
#include "nc_cnn_aiware_runtime.h"
#include "nc_cnn_config_parser.h"

#include "nc_cnn_communicator.h"

#ifdef AIWARE_EMULATOR_SUPPORTED
#include "aiware/emulator/c/emulatordevice.h"
#include "aiware/emulator/c/emulatorconfigs.h"
#endif

#ifdef AIWARE_DEVICE_SUPPORTED
#include "aiware/runtime/c/aiwaredevice.h"
#endif

// #define VERBOSE(f_, ...) printf((f_), ##__VA_ARGS__)
#define VERBOSE(f_, ...)

// #define SHOW_CNN_TIME

#define EACH_PROGRAM_INST_CNT ((int)1)

static int                  g_networks_num = 0;
aiwDevice                   *g_aiw_device = NULL;
static aiwProgramSetBuilder *g_aiwProgSetbuilder = NULL;
struct cnn_config           *g_cnn_config_list[MAX_NETWORK_IDS] ={0,};
static aiwModuleVersion     aiwRuntimeVerInfo;

// Tries to open a physical device, or an emulator if it wasn't successful, and returns its
// pointer. The caller will be the owner of the returned object.
aiwDevice* openDevice()
{
    aiwDevice* device = NULL;

    // If real device support is available on the current platform, then try to open one
    // physical device.
#ifdef AIWARE_DEVICE_SUPPORTED
    VERBOSE("%s", "try to get real device\n");
    // aiwDeviceCount returns the number of physical devices presented in the current
    // system.
    if (aiwDeviceCount() > 0)
    {
        device = aiwDeviceOpen(0);
        if (!device)
        {
            fprintf(stderr, "Failed to open device #0\n");
            return NULL;
        }
        else
        {
            return device;
        }
    }
#endif

    // If real device support is not available, or there isn't any physical device in the
    // system, and if we have emulator support, then try to create an emulator.
#ifdef AIWARE_EMULATOR_SUPPORTED
    aiwDeviceConfigData* config = NULL;
    VERBOSE("try to get emulator\n");
    // First we have to define which kind of emulator we want to create. This is a
    // predefined configuration for APACHE6.
    config = aiwCreateApache6ESConfigDefault();
    if (!config)
    {
        fprintf(stderr, "Failed to get APACHE6 device configuration\n");
        return NULL;
    }

    device = aiwCreateEmulator(config);

    // Config is no longer needed.
    aiwReleaseDeviceConfigData(config);

#endif

    return device;
}

int nc_aiw_init_cnn(void)
{
    aiwModuleVersion aiwModuleVerInfo;

    mq_unlink(MQ_NAME_DATA);

    aiwRuntimeVerInfo = aiwRuntimeVersion();
    printf("aiWare Runtime version:  %s\n", aiwRuntimeVerInfo.version);
    aiwModuleVerInfo = aiwCommonVersion();
    printf("aiwModule Version: %s\n", aiwModuleVerInfo.version);
    printf("Opening device\n");
    g_aiw_device = openDevice();
    if (!g_aiw_device) {
        fprintf(stderr, "Failed to open device\n");
        return -1;
    }
    g_aiwProgSetbuilder = aiwCreateProgramSetBuilder(g_aiw_device);
    if (!g_aiwProgSetbuilder) {
        fprintf(stderr, "Failed to create ProgramSetBuilder on the device\n");
        return -1;
    }
    g_networks_num = 0;
    for (int i = 0; i < MAX_NETWORK_IDS; i++) {
        g_cnn_config_list[i] = NULL;
    }
    return 0;
}

int nc_aiw_add_network_to_builder(const char *path, E_NETWORK_UID uid, PtrFuncPostprc ptr_pp_func)
{
    aiw_status status = AIW_ERROR;
    aiwBinary *binary_data = NULL;

    printf("Loading binary_data : %s\n", path);
    binary_data = aiwBinaryLoadFromPath(path);
    if (!binary_data) {
        fprintf(stderr, "Failed to load binary_data\n");
        return -1;
    }
    status = aiwProgramSetBuilderAddBinaryCopy(g_aiwProgSetbuilder, binary_data, EACH_PROGRAM_INST_CNT);
    if (status != AIW_SUCCESS) {
        fprintf(stderr, "Failed to aiwProgramSetBuilderAddBinaryCopy\n");
        return -1;
    }
    g_cnn_config_list[(int)uid] = (struct cnn_config *)malloc(sizeof(struct cnn_config));
    if (g_cnn_config_list[(int)uid] == NULL) {
        fprintf(stderr, "config list creation failure\n");
        return -1;
    }
    g_cnn_config_list[(int)uid]->network_id = uid;
    g_cnn_config_list[(int)uid]->program_idx = g_networks_num++;
    g_cnn_config_list[(int)uid]->program = NULL;
    g_cnn_config_list[(int)uid]->callback_pp = ptr_pp_func;

    if (nc_set_network_info(path, uid) < 0){
        printf("Failed to set network info\n");
        return -1;
    }
    return g_networks_num;
}

static int nc_get_cnn_network_input_tinfo(struct cnn_config* config)
{
    aiw_u32_t tensor_count = 0;
    // aiwTensor *tensor = NULL;
    const aiwTensorInfo *tinfo = NULL;

    tensor_count = aiwProgramGetInputTensorCount(config->program);
    if(tensor_count != 1)
    {
        fprintf(stderr, "[Warning]Invalid input tensor count\n");
        return -1;
    }

    for (aiw_u32_t i = 0; i < tensor_count; ++i)
    {
        // Get the input tensor
        config->input_tensor = aiwProgramGetInputTensor(config->program, i);
        if (!config->input_tensor)
        {
            fprintf(stderr, "Invalid input tensor object\n");
            return -1;
        }

        // Get the corresponding information object
        tinfo = aiwTensorGetInfo(config->input_tensor);
        if (!tinfo)
        {
            fprintf(stderr, "Failed to get info for tensor\n");
            return -1;
        }
        memcpy(&config->input_tinfo, tinfo, sizeof(aiwTensorInfo));
    }

    return 0;
}


int nc_aiw_finish_network_builder(void)
{
    aiw_status status = AIW_ERROR;
    aiwProgramSet* prog_set = NULL;

    if (g_networks_num == 0) {
        fprintf(stderr, "network does not exist!\n");
        return -1;
    }
    prog_set = aiwProgramSetBuilderFinish(g_aiwProgSetbuilder);
    if (!prog_set) {
        fprintf(stderr, "Failed to aiwProgramSetBuilderFinish\n");
        return -1;
    }

    status = aiwReleaseProgramSetBuilder(g_aiwProgSetbuilder);
    if (status != AIW_SUCCESS) {
        fprintf(stderr, "Failed to aiwReleaseProgramSetBuilder\n");
        return -1;
    }

    for (int i = 0; i < MAX_NETWORK_IDS; i++) {
        if (g_cnn_config_list[i] != NULL) {
            g_cnn_config_list[i]->program = aiwProgramSetGetProgram(prog_set,
                        g_cnn_config_list[i]->program_idx);
            if(nc_get_cnn_network_input_tinfo(g_cnn_config_list[i]) < 0)
            {
                fprintf(stderr, "Failed to nc_get_cnn_network_input_tinfo\n");
                return -1;
            }
        }
    }
    return 0;
}

int nc_get_cnn_networks_num(void)
{
    return g_networks_num;
}

int nc_get_cnn_networks_id(void)
{
    int net_id = -1;

    if(g_networks_num == 0) {
        fprintf(stderr, "network does not exist!\n");
        return -1;
    }
    for (int i = 0; i < MAX_NETWORK_IDS; i++) {
        if (g_cnn_config_list[i] != NULL) {
            net_id = g_cnn_config_list[i]->network_id;
        }
    }
    return net_id;
}

int nc_get_cnn_network_input_resol(int uid, aiwTensorInfo *tinfo)
{
    if(!tinfo) {
        fprintf(stderr, "failed to get NPU network input tensor information\n");
        return -1;
    }
    if(!g_cnn_config_list[uid]) {
        fprintf(stderr, "failed to get NPU network input resolution (uid=%d)\n", uid);
        return -1;
    }

    memcpy(tinfo, &g_cnn_config_list[uid]->input_tinfo, sizeof(aiwTensorInfo));

    return 0;
}
aiw_i8_t is_float_or_uint8(E_NETWORK_UID  net_id, int tensor_index)
{
    aiw_i8_t net_type = -1;
    if(net_id == NETWORK_TRI_CHIMERA){
        if(tensor_index < 3 || tensor_index == 6 || tensor_index == 9){
            net_type = FLOAT_TYPE;
        }
        else{
            net_type = UINT8_TYPE;
        }
    }
    if(net_id == NETWORK_YOLOV8_DET){
        net_type = FLOAT_TYPE;
    }
    else if(net_id == NETWORK_YOLOV5_DET){
        net_type = FLOAT_TYPE;
    }
    else if(net_id == NETWORK_PELEE_SEG){
        net_type = UINT8_TYPE;
    }
    else if(net_id == NETWORK_PELEE_DET){
        net_type = FLOAT_TYPE;
    }
    else if(net_id == NETWORK_UFLD_LANE){
        if(tensor_index == 2 || tensor_index == 5){
            net_type = FLOAT_TYPE;
        }
        else{
            net_type = UINT8_TYPE;
        }
    }
    else{

    }

    return net_type;
}
static void *run_cnn_program(struct cnn_config* config, unsigned char *planar_rgb, uint64_t time_stamp, uint32_t cam_ch, E_NETWORK_UID net_id)
{
    aiw_u32_t tensor_count = 0;
    aiwTensor *tensor = NULL;
    const aiwTensorInfo *tinfo = NULL;
    aiw_u64_t buffer_size = 0;
    aiw_status status = AIW_ERROR;
    void *inference_output = NULL;
    aiwTensorInfo *send_tinfo = NULL;
    aiw_u8_t* buffer = NULL;
    cnn_output_info cnn_output;
    aiw_u8_t net_datatype = 0;
#ifdef SHOW_CNN_TIME
    uint64_t start_time = 0;
    uint64_t elapsed_ms = 0;

    start_time = nc_get_mono_time();
#endif
    if(!planar_rgb)
    {
        fprintf(stderr, "Failed to get planar rgb image\n");
        exit(0);
    }
    status = aiwTensorSetDataNCHWUInt8(config->input_tensor, planar_rgb);
     if (status != AIW_SUCCESS)
     {
         fprintf(stderr, "Failed to initialize input tensor with random data\n");
         exit(0);
     }
    // Upload the input tensors' content from the host memory into aiWare's memory.
    // Please note that this step is always required, even when the host and aiWare share
    // the same memory. In that case CPU cache is flushed to make sure the latest data will
    // be seen by aiWare.
    status = aiwProgramUploadInputs(config->program);
    if (status != AIW_SUCCESS)
    {
        fprintf(stderr, "Failed to upload input into aiWare's memory\n");
        exit(0);
    }

    // printf("+++Exe\n");
    nc_write_reg(0x06804800, 0x000000FF);
    status = aiwProgramExecute(config->program);
    if (status != AIW_SUCCESS)
    {
        fprintf(stderr, "Failed to execute program\n");
        exit(0);
    }
    // printf("---Exe\n");
    nc_write_reg(0x06804800, 0x00000000);

    // Now download the result from aiWare's memory into host buffers.
    // Like aiwProgramUploadInputs, this step is always required.
    status = aiwProgramDownloadOutputs(config->program);
    if (status != AIW_SUCCESS)
    {
        fprintf(stderr, "Failed to download output from aiWare's memory\n");
        exit(0);
    }

    VERBOSE("%s", "Print output tensor info and save the tensors into files\n");
    tensor_count = aiwProgramGetOutputTensorCount(config->program);
    if (tensor_count == 0)
    {
        fprintf(stderr, "Invalid output tensor count %d\n", tensor_count);
        exit(0);
    }

    VERBOSE("tensor_count = %d\n", tensor_count);
    cnn_output.total_tensor_cnt = tensor_count;
    for (aiw_u32_t i = 0; i < tensor_count; ++i)
    {
        // uint64_t s_time = nc_get_mono_us_time();
        tensor = aiwProgramGetOutputTensor(config->program, i);
        if (!tensor)
        {
            fprintf(stderr, "Invalid output tensor object\n");
            exit(0);
        }

        tinfo = aiwTensorGetInfo(tensor);
        if (!tinfo)
        {
            fprintf(stderr, "Failed to get info for tensor\n");
            exit(0);
        }

        VERBOSE("Output tensor #%u\n", i);
        VERBOSE("Name: %s\n", tinfo->name);
        VERBOSE("Dimension: %u x %u x %u\n", tinfo->dim.w, tinfo->dim.h, tinfo->dim.ch);
        // printf("cam%d Dimension: %u x %u x %u\n", cam_ch, tinfo->dim.w, tinfo->dim.h, tinfo->dim.ch);
        cnn_output.index_of_total = i;
        cnn_output.cam_ch = cam_ch;
        cnn_output.width = tinfo->dim.w;
        cnn_output.height = tinfo->dim.h;
        cnn_output.channel = tinfo->dim.ch;
        cnn_output.data_type = tinfo->exponent?CNN_TILED_DT_FLOAT:CNN_TILED_DT_U8;
        cnn_output.net_id = net_id;

        // copy tinfo
        send_tinfo = (aiwTensorInfo *)malloc(sizeof(aiwTensorInfo));
        memset((void*)send_tinfo, 0, sizeof(aiwTensorInfo));
        memcpy(send_tinfo, tinfo, sizeof(aiwTensorInfo));
        cnn_output.tinfo = (aiwTensorInfo *)send_tinfo;

        nc_get_cnn_network_input_resol(config->network_id, &cnn_output.tinfo_in);
        buffer_size = aiwTensorSizeNCHW(tensor);
        net_datatype = is_float_or_uint8(net_id, i) < 0 ? tinfo->exponent : is_float_or_uint8(net_id, i);
        buffer = aiwTensorAcquireRawBufferPointer(tensor);
        if (!buffer)
        {
            fprintf(stderr, "Failed to get tensor buffer pointer\n");
            exit(0);
        }


        if (net_datatype) { // data type : float
            // printf("## EXPONENT\n");
            buffer_size = buffer_size * sizeof(float);
            // printf("buffer size=%d \n", buffer_size);

            inference_output = (float*)malloc(buffer_size);
            if (!inference_output)
            {
                fprintf(stderr, "Failed to allocate output float buffer\n");
                exit(0);
            }
            memset(inference_output, 0, buffer_size);
            if (nc_neon_get_data_NCHW_float_cellrow(cnn_output.tinfo, buffer, (float *)inference_output) != AIW_SUCCESS)
            {
                fprintf(stderr, "Failed to get output float tensor data\n");
                exit(0);
            }
        } else { // data type : uint8_t
            // seg의 buffer_size는 dim.w * dim.h *dim.ch 가 아닌 tile size로 계산
            // printf("buffer size=%d \n", buffer_size);

            inference_output = (aiw_u8_t *)malloc(buffer_size);
            if (!inference_output)
            {
                fprintf(stderr, "Failed to allocate output uint8 buffer\n");
                exit(0);
            }
            memset(inference_output, 0, buffer_size);

            if (aiwTensorGetDataNCHWUInt8(tensor, (aiw_u8_t *)inference_output) != AIW_SUCCESS)
            {
                fprintf(stderr, "Failed to get output uint8 tensor data\n");
                exit(0);
            }
        }

        status = aiwTensorReleaseRawBufferPointer(tensor);
        if (status != AIW_SUCCESS) {
            fprintf(stderr, "Failed to release tensor buffer\n");
            exit(0);
        }

        tensor = NULL;
        buffer = NULL;
        cnn_output.tiled_data = inference_output;
        // printf("......... tensor(%d) elapsed time : %llu us\n", i, nc_elapsed_us_time(s_time));

        // s_time = nc_get_mono_us_time();
        nc_cnn_send_inference_result(config, time_stamp, cnn_output);
        // printf("......... nc_cnn_send_inference_result() elapsed time : %llu us\n", nc_elapsed_us_time(s_time));
    }

#ifdef SHOW_CNN_TIME
    static uint64_t max_e_time = 0;
    static double avg_time = 0.0;
    static uint64_t t_time = 0;
    static uint64_t f_cnt = 0;
    static uint64_t over_66_cnt = 0;
    elapsed_ms = nc_elapsed_time(start_time);

    f_cnt++;
    t_time += elapsed_ms;
    avg_time = (double)t_time/f_cnt;
    if (f_cnt > 15*10 && elapsed_ms > max_e_time) {
        max_e_time = elapsed_ms;
    }
    if (elapsed_ms > 66) over_66_cnt++;
    printf("(RUN_CNN %d) elapsed time: %llu ms (max: %llu ms, avg: %0.1f ms)\n", \
            config->network_id, elapsed_ms, max_e_time, avg_time);
#endif
    return inference_output;
}

void nc_aiw_run_cnn(unsigned char *planar_rgb, uint64_t time_stamp, uint32_t cam_ch, E_NETWORK_UID net_id)
{
    VERBOSE("entering %s\n", __func__);

    if (g_cnn_config_list[(int)net_id] != NULL){
        run_cnn_program(g_cnn_config_list[(int)net_id], planar_rgb, time_stamp, cam_ch, net_id);
     }
}
