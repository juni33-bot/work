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
* @file    : jsonc_writer.c 
*
* @brief   : Implementation of json file
*
* @author  : AI SW team.  NextChip Inc.  
*
* @date    : 2024.03.18.
*
* @version : 1.0.0
********************************************************************************
* @note
* 2024.03.18 / 1.0.0 / Initial released.
* 
********************************************************************************
*/ 

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <stdint.h>

#include "nc_cnn_aiware_runtime.h"
#include "json.h"
#include "jsonc_writer.h"

static void prepared_line_file(char* line, unsigned int cnt, char *json_file_name)
{
    static FILE* fp;

    fp = fopen(json_file_name, "a");

    fwrite(line, sizeof(char), strlen(line), fp);

    fclose(fp);
}

#if 1
void add_array_box(chn_t *chn, json_object * arr)
{
    unsigned int i=0;

    for(i=0; i<chn->box_cnt; i++)
    {
        json_object *obj_box = json_object_new_object();
        json_object_object_add(obj_box, "X", json_object_new_int(chn->box[i].x));
        json_object_object_add(obj_box, "Y", json_object_new_int(chn->box[i].y));
        json_object_object_add(obj_box, "W", json_object_new_int(chn->box[i].w));
        json_object_object_add(obj_box, "H", json_object_new_int(chn->box[i].h));
        json_object_object_add(obj_box, "S", json_object_new_double(chn->box[i].s));
        json_object_array_add(arr, obj_box);
    }
}

#if 1
/*
 * json structure
[
{
	"FrameCnt" : 0,
	"Class" : [
		{
			"Channel" : 0,
			"BoxCnt" : 2,
			"Box" : [ 
                {"X":1164, "Y":465, "W":66, "H":42, "S":0.99},
                {"X":14, "Y":45, "W":36, "H":22, "S":0.97}
            ]
		},
		{
			"Channel" : 1,
			"BoxCnt" : 0,
			"Box" : [ ]
		},
		{
			"Channel" : 2,
			"BoxCnt" : 0,
			"Box" : [ ]
		}
	]
},
{
	"FrameCnt" : 0,
	"Class" : [
		{
			"Channel" : 0,
			"BoxCnt" : 0,
			"Box" : [ ]
		},
		{
			"Channel" : 1,
			"BoxCnt" : 0,
			"Box" : [ ]
		},
		{
			"Channel" : 2,
			"BoxCnt" : 0,
			"Box" : [ ]
		}
	]
},
........
........
]
*/

// 매 frame rootarr에 BBox 정보를 저장하고, 마지막 frame이 끝나고 rootarr를 파일로 한번에 저장한다.
json_object *rootarr;
int make_json(frame_info_t *frame, char *result_folder_name)
{
    // Will hold the string representation of the json object
    char jsonfilename[255] = {0,};
    char *serialized_json;
    unsigned int i=0;
    unsigned int channel=0;
    //json_object *root = json_object_new_object();

    // Create an empty object : {}
    json_object *obj = json_object_new_object();

    json_object_object_add(obj, "FrameCnt", json_object_new_int(frame->cnt)); // Add an object field to a obj ("FrameCnt": int val,)

    json_object *class_array = json_object_new_array(); // Create a class array
    for(channel=0; channel<VALID_CHANNEL_CNT; channel++)
    {
        json_object * obj_class = json_object_new_object(); // class array에 담을 object 생성
        json_object_object_add(obj_class, "Channel", json_object_new_int(channel)); // class object에 "Channel" obj 추가
        json_object_object_add(obj_class, "BoxCnt", json_object_new_int(frame->chn[channel].box_cnt));
        
        json_object *box_array = json_object_new_array();
        
        add_array_box(&frame->chn[channel], box_array);
        json_object_object_add(obj_class, "Box", box_array);  // Box array를 obj_class에 추가
        json_object_array_add(class_array, obj_class);  // class obj를  class array에 추가
    }

    json_object_object_add(obj,"Class", class_array); // class array를 obj에 추가. 

    if(frame->cnt == 0)
    {
        rootarr = json_object_new_array();
    }
    json_object_array_add(rootarr, obj);

    if(frame->cnt == frame->total_file_cnt-1)
    {
        sprintf(jsonfilename, "%s_%d.json", result_folder_name, frame->cnt);
        serialized_json = (char *)json_object_to_json_string(rootarr);
        prepared_line_file(serialized_json, frame->cnt, jsonfilename);
        printf("save json file\n");
    
        json_object_put(obj);      
    }
    //json_object_put(root); 
    //json_object_put(obj); 
    //printf("json--\n");

    return EXIT_SUCCESS;
}

// 매 프레임마다 BBox 정보를 file에  append
int make_json_separate(frame_info_t *frame, char *result_folder_name)
{
    // Will hold the string representation of the json object
    char jsonfilename[255] = {0,};
    char *serialized_json;
    unsigned int i=0;
    unsigned int channel=0;

    // Create an empty object : {}
    json_object *root = json_object_new_object();
    json_object *obj = json_object_new_object();
    //printf("json++\n");

    json_object_object_add(obj,"FrameCnt", json_object_new_int(frame->cnt));

    json_object *class_array = json_object_new_array();
    for(channel=0; channel<CHANNEL_CNT; channel++)
    {
        json_object * obj_class = json_object_new_object();
        json_object_object_add(obj_class, "Channel", json_object_new_int(channel));
        json_object_object_add(obj_class, "BoxCnt", json_object_new_int(frame->chn[channel].box_cnt));
        
        json_object *arr = json_object_new_array();
        
        add_array_box(&frame->chn[channel], arr);
        json_object_object_add(obj_class, "Box", arr);
        json_object_array_add(class_array, obj_class);
    }

    json_object_object_add(obj,"Class", class_array);

    //json_object *rootarr = json_object_new_array();
    //json_object_array_add(rootarr, obj);
    //serialized_json = (char *)json_object_to_json_string(rootarr);

#if 0
    serialized_json = (char *)json_object_to_json_string(obj);
    //printf("%s\n", serialized_json);

    prepared_line_file(serialized_json, frame->cnt);
#else
    sprintf(jsonfilename, "%s_%d.json", result_folder_name, frame->cnt);
    json_object_to_file(jsonfilename, obj);
    printf("save json file\n");
#endif
    json_object_put(obj); 
    //printf("json--\n");

    return EXIT_SUCCESS;
}

#else
int make_json(frame_info_t *frame)
{
    int chn_cnt=0;
    int b_cnt=0;
    static FILE *fp;
    char fname[255];

#ifdef USE_JSON_EACH_FILE
    sprintf(fname, "%s_%d.json", DET_FILENAME, frame->cnt);
    fp = fopen(fname, "w");
#else
    if( frame->cnt == 1)
    {
        // create a json file
        sprintf(fname, "%s_all.json", DET_FILENAME);
        fp = fopen(fname, "w");
    }
#endif
    if(fp == NULL)
    {
        printf("Cannot open file: %s\n", fname);
    }
#ifdef USE_JSON_EACH_FILE
#else
    if(frame->cnt == 1) {
    fprintf(fp, "[");
    }
#endif

    fprintf(fp, "{\n");
    fprintf(fp, "   \"FrameCnt\": %d,\n", frame->cnt);
    fprintf(fp, "   \"Class\": [\n");
    for(chn_cnt=0; chn_cnt<CHANNEL_CNT; chn_cnt++)
    {
    fprintf(fp, "       {\n");
    fprintf(fp, "         \"Channel\": %d,\n", chn_cnt);
    fprintf(fp, "         \"BoxCnt\": %d,\n", frame->chn[chn_cnt].box_cnt);
    if(frame->chn[chn_cnt].box_cnt > 0)
    {
    fprintf(fp, "         \"Box\": [\n");
        for(b_cnt=0; b_cnt<frame->chn[chn_cnt].box_cnt;b_cnt++)
        {
            if(b_cnt == frame->chn[chn_cnt].box_cnt-1)  // last line
            {
    fprintf(fp, "           {\"X\":%d, \"Y\":%d, \"W\":%d, \"H\":%d, \"S\":%f}\n", 
                        frame->chn[chn_cnt].box[b_cnt].x, frame->chn[chn_cnt].box[b_cnt].y,
                        frame->chn[chn_cnt].box[b_cnt].w, frame->chn[chn_cnt].box[b_cnt].h,
                        frame->chn[chn_cnt].box[b_cnt].s);
            } else {
    fprintf(fp, "           {\"X\":%d, \"Y\":%d, \"W\":%d, \"H\":%d, \"S\":%f},\n", 
                        frame->chn[chn_cnt].box[b_cnt].x, frame->chn[chn_cnt].box[b_cnt].y,
                        frame->chn[chn_cnt].box[b_cnt].w, frame->chn[chn_cnt].box[b_cnt].h,
                        frame->chn[chn_cnt].box[b_cnt].s); // The last line doesn't need ','
            }
        }
    fprintf(fp, "         ]\n");  // Box
    }
    else {
    fprintf(fp, "         \"Box\": [ ]\n");
    }
    if(chn_cnt == CHANNEL_CNT-1) { // last line
    fprintf(fp, "       }\n");

    } else {
    fprintf(fp, "       },\n");
    }
    } //chn_cnt
    fprintf(fp, "   ]\n");  // class

    if(frame_last || frame->cnt >= MAX_FILE_COUNT) {
    fprintf(fp, "}"); // The last line doesn't need ','
    fprintf(fp, "]");

    } else {
    fprintf(fp, "},");
    }
    //fprintf(fp, "},\n");

#ifdef USE_JSON_EACH_FILE
    fclose(fp);
#else
    if(frame->cnt >= MAX_FILE_COUNT)
    {
        fclose(fp);
    }
#endif

    return 0;
}
#endif
#endif

