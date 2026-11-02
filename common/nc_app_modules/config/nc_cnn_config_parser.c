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
* @file    : nc_cnn_config_parser.c
*
* @brief   : implementation of nc cnn config parser
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

/*
********************************************************************************
*               INCLUDES
********************************************************************************
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "iniLib/ini.h"
#include "nc_cnn_config_parser.h"
#include "nc_utils.h"
#include "nc_cnn_network_includes.h"


/*
********************************************************************************
*               DEFINES
********************************************************************************
*/

#define MAX_FILE_NAME_LENGTH    (200)
#define MAX_PATH_LENGTH         (256)
#define MAX_CLASS_NAME_LEN      (10)       // each class name length

#define MATCH(s, n) ((strcmp(section, s) == 0) && (strcmp(name, n) == 0))

/*
********************************************************************************
*               FUNCTION DEFINITIONS
********************************************************************************
*/

stNetwork_info *net_info[MAX_NETWORK_IDS] = {0,};


static int handle_detection(stConfig_net* pConfig, const char* name, const char* value) {
    if (strcmp(name, "class_num") == 0) {
        pConfig->class_num = atoi(value);
    } else if (strcmp(name, "class_name") == 0) {
        memset(pConfig->class_name, 0, MAX_TOT_CLASS_NAME_LEN);
        strncpy(pConfig->class_name, value, MAX_TOT_CLASS_NAME_LEN - 1);
    } else if (strcmp(name, "class_color") == 0) {
        memset(pConfig->class_color, 0, MAX_CLASS_COLOR_LEN);
        strncpy(pConfig->class_color, value, MAX_CLASS_COLOR_LEN - 1);
    } else if (strcmp(name, "anchor_num") == 0) {
        pConfig->anchor_num = atoi(value);
    } else if (strcmp(name, "anchor_size") == 0) {
        memset(pConfig->anchor_size, 0, MAX_ANCHOR_LEN);
        strncpy(pConfig->anchor_size, value, MAX_ANCHOR_LEN - 1);
    } else if (strcmp(name, "CONFIDENCE_OBJECTNESS_THRESHOLD") == 0) {
        pConfig->obj_th = atof(value);
    } else if (strcmp(name, "CONFIDENCE_DETECTION_THRESHOLD") == 0) {
        pConfig->det_th = atof(value);
    } else if (strcmp(name, "CONFIDENCE_NMS_THRESHOLD") == 0) {
        pConfig->nms_th = atof(value);
    } else {
        return 0;  /* unknown name */
    }
    return 1;  /* handled */
}

static int handle_segmentation(stConfig_net* pConfig, const char* name, const char* value) {
    if (strcmp(name, "class_num") == 0) {
        pConfig->seg_class_num = atoi(value);
    } else if (strcmp(name, "class_name") == 0) {
        memset(pConfig->seg_class_name, 0, MAX_TOT_CLASS_NAME_LEN);
        strncpy(pConfig->seg_class_name, value, MAX_TOT_CLASS_NAME_LEN - 1);
    } else if (strcmp(name, "class_color") == 0) {
        memset(pConfig->seg_class_color, 0, MAX_CLASS_COLOR_LEN);
        strncpy(pConfig->seg_class_color, value, MAX_CLASS_COLOR_LEN - 1);
    } else {
        return 0;  /* unknown name */
    }
    return 1;  /* handled */
}

static int handle_lane(stConfig_net* pConfig, const char* name, const char* value) {
    if (strcmp(name, "max_lane_num") == 0) {
        pConfig->lane_max_num = atoi(value);
    } else if (strcmp(name, "lane_anchor_info") == 0) {
        memset(pConfig->lane_anchor_info, 0, MAX_LANE_INFO_LEN);
        strncpy(pConfig->lane_anchor_info, value, MAX_LANE_INFO_LEN - 1);
    } else if (strcmp(name, "draw_color") == 0) {
        memset(pConfig->lane_draw_color, 0, MAX_CLASS_COLOR_LEN);
        strncpy(pConfig->lane_draw_color, value, MAX_CLASS_COLOR_LEN - 1);
    } else if (strcmp(name, "row_anchor_num") == 0) {
        pConfig->lane_row_anchor_num = atoi(value);
    } else if (strcmp(name, "col_anchor_num") == 0) {
        pConfig->lane_col_anchor_num = atoi(value);
    } else if (strcmp(name, "row_anchor_min") == 0) {
       pConfig->lane_row_anchor_min = (float)atof(value);
    } else if (strcmp(name, "row_anchor_max") == 0) {
        pConfig->lane_row_anchor_max = (float)atof(value);
    } else if (strcmp(name, "col_anchor_min") == 0) {
        pConfig->lane_col_anchor_min = (float)atof(value);
    } else if (strcmp(name, "col_anchor_max") == 0) {
        pConfig->lane_col_anchor_max = (float)atof(value);
    } else if (strcmp(name, "row_threshold") == 0) {
        pConfig->lane_row_th = atoi(value);
    } else if (strcmp(name, "col_threshold") == 0) {
        pConfig->lane_col_th = atoi(value);
    }
#ifdef USE_UFLD_NETWORK_DEBUGGING
    else if (strcmp(name, "row_cell_num") == 0) {
        pConfig->lane_row_cell_num = atoi(value);
    } else if (strcmp(name, "col_cell_num") == 0) {
        pConfig->lane_col_cell_num = atoi(value);
    }
#endif
    else {
        return 0;  /* unknown name */
    }
    return 1;  /* handled */
}

static int nc_ini_net_handler(void* user, const char* section, const char* name, const char* value) {
    stConfig_net* pConfig = (stConfig_net*)user;

    if (strcmp(section, "detection") == 0) {
        handle_detection(pConfig, name, value);
    } else if (strcmp(section, "segmentation") == 0) {
        handle_segmentation(pConfig, name, value);
    } else if (strcmp(section, "lane") == 0) {
        handle_lane(pConfig, name, value);
    } else if (strcmp(section, "multi") == 0) {
        handle_detection(pConfig, name, value);
        handle_segmentation(pConfig, name, value);
        handle_lane(pConfig, name, value);
    } else {
        return 0;  /* unknown section */
    }
    return 0;
}

int multi_config_parse(char* full_path, void *config) {

    int j = 0;
    char line[MAX_PATH_LENGTH];
    char ini_path[MAX_PATH_LENGTH];
    const char* task[] = {"detection", "segmentation", "lane"};
    const int num_tasks = sizeof(task) / sizeof(task[0]);

    FILE *file = fopen(full_path, "r");
    if (!file) {
        printf("Can't load %s\n", full_path);
        return 1;
    }
    while (fgets(line, sizeof(line), file)) {        
        for(int i = 0; i < num_tasks ; ++i){
            size_t key_len = strlen(task[i]);
            if (strncmp(line, task[i], key_len) == 0) {
                if (sscanf(line, "%*[^=]= %s", ini_path) == 1) {
                    if (ini_parse(ini_path, nc_ini_net_handler, config) < 0) {
                        printf("Can't load %s\n", ini_path);
                        return -1;
                    }
                }
                j++;
            }
        }
    }
    if(j < 2){
        printf("Multi-network must have at least two ini file paths.\n");
        return -1;
    }
    fclose(file);

    return 0;
}

int nc_net_config_parse (char * full_path, stNetwork_info* net_info)
{
    stConfig_net config; // string, unrefined data

    int i = 0, j = 0;
    char* ptr = NULL;
    char delimeter[] = ",(){}[] ";
    
    memset(&config, 0, sizeof(stConfig_net));
    /* Load Config from ini file */
    if(net_info->network_id == NETWORK_TRI_CHIMERA){
        if (multi_config_parse(full_path, &config) < 0) return -1;
    }
    else{
        if (ini_parse(full_path, nc_ini_net_handler, &config) < 0) {
            printf("Can't load %s\n", full_path);
            return -1;
        }
    }
    /**************** detection parsing ****************/
    net_info->class_id = (uint16_t *)malloc(sizeof(uint16_t) * config.class_num);
    net_info->class_name = (char**)malloc(sizeof(char*) * config.class_num);
    net_info->class_name[0] = (char *)malloc(sizeof(char) * config.class_num * MAX_CLASS_NAME_LEN);
    net_info->class_color = (stRGB*)malloc(sizeof(stRGB) * config.class_num);
    net_info->anchor_size = (float*)malloc(sizeof(float) * 2 * config.anchor_num);

    if((net_info->class_name == NULL) || (net_info->class_name[0] == NULL) || (net_info->class_id == NULL) || (net_info->class_color == NULL) || (net_info->anchor_size == NULL)){
        printf("Can't alloc memory for net_info\n");
        return -1;
    }

    /**************** make class id array ****************/
    for(i=0; i<config.class_num;i++)
    {
        net_info->class_id[i]=(uint16_t)i;
    }


    /**************** class name parsing ****************/
    for(i=1; i< config.class_num; i++)
    {
        net_info->class_name[i] =  net_info->class_name[i-1] + MAX_CLASS_NAME_LEN;
    }

    ptr = strtok(config.class_name,delimeter);
    for(i=0; i<config.class_num; i++)
    {
        if(ptr == NULL){
            printf("[NETWORK INI ERROR] class_name_num < class_num\n");
            return -1;
        }
        strcpy(net_info->class_name[i],ptr);
        ptr = strtok(NULL,delimeter);
    }
    if(ptr != NULL){
        printf("[NETWORK INI ERROR] class_name_num > class_num\n");
        return -1;
    }

    /**************** class color parsing ****************/
    ptr = strtok(config.class_color, delimeter);
    for(i=0; i<config.class_num; i++)
    {
        for(j=0; j <3 ; j++)
        {
            if(ptr == NULL){
                printf("[NETWORK INI ERROR] class_color_num < class_num\n");
                return -1;
            }
            if (j==0) net_info->class_color[i].r = (uint8_t)atoi(ptr);
            if (j==1) net_info->class_color[i].g = (uint8_t)atoi(ptr);
            if (j==2) net_info->class_color[i].b = (uint8_t)atoi(ptr);
            ptr = strtok(NULL,delimeter);
        }
    }
    if(ptr != NULL){
        printf("[NETWORK INI ERROR] class_color_num > class_num\n");
        return -1;
    }


    /**************** anchor size parsing ****************/
    ptr = strtok(config.anchor_size,delimeter);
    for(i=0; i<config.anchor_num; i++)
    {
        for(j=0; j <2 ; j++)
        {
            if(ptr == NULL){
                printf("[NETWORK INI ERROR] anchor_size_num < anchor_num\n");
                return -1;
            }
            if(j==0) net_info->anchor_size[i] = (float)atof(ptr);
            if(j==1) net_info->anchor_size[i+config.anchor_num] = (float)atof(ptr);
            ptr = strtok(NULL,delimeter);
        }
    }
    if(ptr != NULL){
        printf("[NETWORK INI ERROR] anchor_size_num > anchor_num\n");
        return -1;
    }


    net_info->class_num = config.class_num;
    net_info->anchor_num = config.anchor_num;
    net_info->obj_th = config.obj_th;
    net_info->det_th = config.det_th;
    net_info->nms_th = config.nms_th;

    /**************** segmentation parsing ****************/
    net_info->seg_class_name = (char**)malloc(sizeof(char*) * config.seg_class_num);
    net_info->seg_class_name[0] = (char *)malloc(sizeof(char) * config.seg_class_num * MAX_CLASS_NAME_LEN);
    net_info->seg_class_color = (stRGBA*)malloc(sizeof(stRGBA) * config.seg_class_num);

    if((net_info->seg_class_name == NULL) || (net_info->seg_class_name[0] == NULL) || (net_info->seg_class_color == NULL)){
        printf("Can't alloc memory for net_info\n");
        return -1;
    }

    /**************** class name parsing ****************/
    for(i=1; i< config.seg_class_num; i++)
    {
        net_info->seg_class_name[i] =  net_info->seg_class_name[i-1] + MAX_CLASS_NAME_LEN;
    }

    ptr = strtok(config.seg_class_name,delimeter);
    for(i=0; i<config.seg_class_num; i++)
    {
        if(ptr == NULL){
            printf("[NETWORK INI ERROR] class_name_num < class_num\n");
            return -1;
        }
        strcpy(net_info->seg_class_name[i],ptr);
        ptr = strtok(NULL,delimeter);
    }
    if(ptr != NULL){
        printf("[NETWORK INI ERROR] class_name_num > class_num\n");
        return -1;
    }

    /**************** class color parsing ****************/
    ptr = strtok(config.seg_class_color, delimeter);
    for(i=0; i<config.seg_class_num; i++)
    {
        for(j=0; j <4 ; j++)
        {
            if(ptr == NULL){
                printf("[NETWORK INI ERROR] class_color_num < class_num\n");
                return -1;
            }
            if (j==0) net_info->seg_class_color[i].r = (uint8_t)atoi(ptr);
            if (j==1) net_info->seg_class_color[i].g = (uint8_t)atoi(ptr);
            if (j==2) net_info->seg_class_color[i].b = (uint8_t)atoi(ptr);
            if (j==3) net_info->seg_class_color[i].a = (uint8_t)atoi(ptr);
            ptr = strtok(NULL,delimeter);
        }
    }
    if(ptr != NULL){
        printf("[NETWORK INI ERROR] class_color_num > class_num\n");
        return -1;
    }



    net_info->seg_class_num = config.seg_class_num;

    if((net_info->obj_th < 0) || (net_info->obj_th > 1)|| (net_info->det_th < 0) || (net_info->det_th > 1) || (net_info->nms_th < 0) || (net_info->nms_th > 1))
    {
        printf("[NETWORK INI WARNING] threshold out of range\n");
    }

    /**************** Lane detection parsing ****************/

    net_info->lane_anchor_info = (int *)malloc(sizeof(int) * config.lane_max_num);
    net_info->lane_draw_color = (stRGB*)malloc(sizeof(stRGB) * config.lane_max_num);
    net_info->lane_row_anchor = (float *)malloc(sizeof(float) * config.lane_row_anchor_num);
    net_info->lane_col_anchor = (float *)malloc(sizeof(float) * config.lane_col_anchor_num);
    
    ptr = strtok(config.lane_anchor_info, delimeter);
    for(i=0; i<config.lane_max_num; i++)
    {
        if(ptr == NULL){
            printf("[NETWORK INI ERROR] num lane_anchor_info < max_lane_num\n");
            return -1;
        }

        if(strcmp(ptr, "row")==0) {
            net_info->lane_anchor_info[i] = ROW_ANCHOR;
        }
        else if(strcmp(ptr, "col")==0) {
            net_info->lane_anchor_info[i] = COL_ANCHOR;
        }
        else {
            printf("[NETWORK INI ERROR] wrong lane anchor info\n");
            return -1;
        }
        ptr = strtok(NULL,delimeter);
    }
    if(ptr != NULL){
        printf("[NETWORK INI ERROR] num lane_anchor_info > max_lane_num\n");
        return -1;
    }

    ptr = strtok(config.lane_draw_color, delimeter);
    for(i=0; i<config.lane_max_num; i++)
    {
        for(j=0; j <3 ; j++)
        {
            if(ptr == NULL){
                printf("[NETWORK INI ERROR] num draw_color < lane_max_num\n");
                return -1;
            }
            if (j==0) net_info->lane_draw_color[i].r = (uint8_t)atoi(ptr);
            if (j==1) net_info->lane_draw_color[i].g = (uint8_t)atoi(ptr);
            if (j==2) net_info->lane_draw_color[i].b = (uint8_t)atoi(ptr);
            ptr = strtok(NULL,delimeter);
        }
    }
    if(ptr != NULL){
        printf("[NETWORK INI ERROR] num draw_color > lane_max_num\n");
        return -1;
    }

    

    float step = (config.lane_row_anchor_max - config.lane_row_anchor_min) / (float)(config.lane_row_anchor_num - 1); 
    for(i=0 ; i<config.lane_row_anchor_num; i++)
    {
        net_info->lane_row_anchor[i] = config.lane_row_anchor_min + (float)i * step;
    }

    step = (config.lane_col_anchor_max - config.lane_col_anchor_min) / (float)(config.lane_col_anchor_num - 1); 
    for(i=0 ; i<config.lane_col_anchor_num; i++)
    {
        net_info->lane_col_anchor[i] = config.lane_col_anchor_min + (float)i * step;
    }

    net_info->lane_max_num = config.lane_max_num;
    net_info->lane_anchor_th[ROW_ANCHOR] = config.lane_row_th;
    net_info->lane_anchor_th[COL_ANCHOR] = config.lane_col_th;
#ifdef USE_UFLD_NETWORK_DEBUGGING
    net_info->lane_row_anchor_num = config.lane_row_anchor_num;
    net_info->lane_col_anchor_num = config.lane_col_anchor_num;
    net_info->lane_row_anchor_min = config.lane_row_anchor_min;
    net_info->lane_row_anchor_max = config.lane_row_anchor_max;
    net_info->lane_col_anchor_min = config.lane_col_anchor_min;
    net_info->lane_col_anchor_max = config.lane_col_anchor_max;
    net_info->lane_row_cell_num = config.lane_row_cell_num;
    net_info->lane_col_cell_num = config.lane_col_cell_num;
#endif
    return 0;
}


void nc_free_network_info(void)
{
    for(int i=0; i<MAX_NETWORK_IDS; i++)
    {
        if(net_info[i] != NULL)
        {
            if(net_info[i]->class_name[0] != NULL) free(net_info[i]->class_name[0]);
            if(net_info[i]->class_name    != NULL) free(net_info[i]->class_name);
            if(net_info[i]->class_color   != NULL) free(net_info[i]->class_color);
            if(net_info[i]->anchor_size   != NULL) free(net_info[i]->anchor_size);
            if(net_info[i]->class_id      != NULL) free(net_info[i]->class_id);
            if(net_info[i]->lane_anchor_info != NULL) free(net_info[i]->lane_anchor_info);
            if(net_info[i]->lane_draw_color  != NULL) free(net_info[i]->lane_draw_color);
            if(net_info[i]->lane_row_anchor  != NULL) free(net_info[i]->lane_row_anchor);
            if(net_info[i]->lane_col_anchor  != NULL) free(net_info[i]->lane_col_anchor);
            free(net_info[i]);
        }
    }
}


int nc_set_network_info (const char* NETWORK_FILE_PATH, E_NETWORK_UID n_id)
{
    char fname[MAX_FILE_NAME_LENGTH];
    char NET_INI_FILE[MAX_PATH_LENGTH];

    const char *startpointer;
    const char *endpointer;

    startpointer = strrchr(NETWORK_FILE_PATH,'/');
    endpointer = strrchr(NETWORK_FILE_PATH,'.');
    strncpy(fname, startpointer + 1, endpointer-startpointer-1);
    fname[endpointer-startpointer-1]='\0';
    sprintf(NET_INI_FILE,"misc/%s.ini",fname);

    net_info[n_id] = (stNetwork_info *)malloc(sizeof(stNetwork_info));
    if (net_info[n_id] == NULL) {
        printf("Can't alloc memory for struct net_info\n");
        nc_free_network_info();
        return -1;
    }

    net_info[n_id]->network_id = n_id;

    if (nc_net_config_parse(nc_localize_path((const char*) NET_INI_FILE),net_info[n_id]) < 0) {
        printf("Network config parse failure <%s.ini>\n", fname);
        nc_free_network_info();
        return -1;
    }

    return 0;
}

stNetwork_info* nc_cnn_get_network_info(E_NETWORK_UID n_id)
{
    stNetwork_info* ret_info = NULL;

    if (net_info[(int)n_id] == NULL) {
        printf("nc_cnn_get_network_info fail. net_id:%d\n", (int)n_id);
        ret_info = NULL;
    } else {
        ret_info = net_info[(int)n_id];
    }

    return ret_info;
}