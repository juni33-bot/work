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
* @file    : nc_cnn_config_parser.h
*
* @brief   : nc cnn config parser header
*
* @author  : ADAS SW team.  NextChip Inc.
*
* @date    : 2023.04.10
*
* @version : 1.0.0
********************************************************************************
* @note
* 2023.04.10 / 1.0.0 / Initial released.
*
********************************************************************************
*/

#ifndef __NC_CNN_CONFIG_PARSER__
#define __NC_CNN_CONFIG_PARSER__


#include "nc_cnn_network_includes.h"
/*
********************************************************************************
               TYPEDEFS
********************************************************************************
*/
#define MAX_TOT_CLASS_NAME_LEN 100
#define MAX_ANCHOR_LEN 100
#define MAX_CLASS_COLOR_LEN 800
#define MAX_LANE_INFO_LEN 100

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} stRGB;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} stRGBA;

typedef enum {
    ROW_ANCHOR,
    COL_ANCHOR,
    NUM_ANCHOR,
} E_LANE_ANCHOR;

typedef struct{
    int class_num;
    char class_name[MAX_TOT_CLASS_NAME_LEN];
    char class_color[MAX_CLASS_COLOR_LEN];
    int anchor_num;
    char anchor_size[MAX_ANCHOR_LEN];

    double obj_th;
    double det_th;
    double nms_th;

    int seg_class_num;
    char seg_class_name[MAX_TOT_CLASS_NAME_LEN];
    char seg_class_color[MAX_CLASS_COLOR_LEN];

    int lane_max_num;
    char lane_anchor_info[MAX_LANE_INFO_LEN];
    char lane_draw_color[MAX_CLASS_COLOR_LEN];
    int lane_row_anchor_num;
    int lane_col_anchor_num;
    float lane_row_anchor_min;
    float lane_row_anchor_max;
    float lane_col_anchor_min;
    float lane_col_anchor_max;
    int lane_row_th;
    int lane_col_th;
#ifdef USE_UFLD_NETWORK_DEBUGGING
    int lane_row_cell_num;
    int lane_col_cell_num;
#endif
} stConfig_net;

typedef struct{
    int network_id;
    int class_num;
    char** class_name;
    stRGB* class_color;
    uint16_t* class_id;
    int anchor_num;
    float* anchor_size;

    double obj_th;
    double det_th;
    double nms_th;

    int seg_class_num;
    char** seg_class_name;
    stRGBA* seg_class_color;

    int lane_max_num;
    int* lane_anchor_info;
    stRGB* lane_draw_color;
    float* lane_row_anchor;
    float* lane_col_anchor;
#ifdef USE_UFLD_NETWORK_DEBUGGING
    int lane_row_anchor_num;
    int lane_col_anchor_num;
    float lane_row_anchor_min;
    float lane_row_anchor_max;
    float lane_col_anchor_min;
    float lane_col_anchor_max;
    int lane_row_cell_num;
    int lane_col_cell_num;
#endif
    int lane_anchor_th[NUM_ANCHOR];

} stNetwork_info;

extern int nc_net_config_parse (char * full_path, stNetwork_info* net_info);
extern int nc_set_network_info (const char* NETWORK_FILE_PATH, E_NETWORK_UID n_id);
extern void nc_free_network_info(void);
extern stNetwork_info* nc_cnn_get_network_info(E_NETWORK_UID n_id);

#endif // __NC_CNN_CONFIG_PARSER__
