/**
********************************************************************************
* Copyright (C) 2021 NEXTCHIP Inc. All rights reserved.
* This software is the confidential and proprietary information of
* NEXTCHIP, Inc. ("Confidential Information"). You shall not disclose such
* Confidential Information and shall use it only in accordance with
* the terms of the license agreement you entered into with NEXTCHIP.
********************************************************************************

********************************************************************************
* @file    : npu_app.c
*
* @brief   : application for CNN works
*
* @author  : AI SW team.  NextChip Inc.
*
* @date    : 2023.06.01.
*
* @version : 1.0.0
********************************************************************************
* @note
* 2023.06.01 / 1.0.0 / Initial released.
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
#include <time.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include "aiware/common/c/binary.h"
#include "aiware/common/c/version.h"
#include "aiware/runtime/c/device.h"
#include "aiware/runtime/c/program.h"
#include "aiware/runtime/c/programset.h"
#include "aiware/runtime/c/aiwaredevice.h"
#include "aiware/runtime/c/version.h"
#include "opencv2/opencv.hpp"
#include "nc_utils.h"
#include "nc_cnn_aiware_runtime.h"

#ifdef USE_POSTPROCESS
#include "nc_cnn_config_parser.h"
#include "nc_cnn_anchor.h"
#include "cairo.h"
#endif

#ifdef SAVE_META_DATA
#include "jsonc_writer.h"
#endif
/*
********************************************************************************
*               DEFINES
********************************************************************************
*/
#define USE_FPS             (30)
#define RGB_CNT             (3)
#define RGBA_CNT            (4)
#define MAX_PATH            (256)
#define MAX_PATH_LENGTH     (256)

#ifdef SAVE_META_DATA
#define STATUS_FILE_NAME    "./status_file.txt"
#endif

/*
********************************************************************************
*               VARIABLE DECLARATIONS
********************************************************************************
*/
aiwDevice *device = NULL;
aiwBinary *binary = NULL;
aiwProgram *program = NULL;
static aiwModuleVersion aiwRuntimeVerInfo;

int network_input_width;
int network_input_height;
int network_input_chanel;
int network_input_batch;
int network_output_width;
int network_output_height;
int network_output_chanel;
int network_output_batch;
aiw_u32_t tensor_count = 0;
unsigned char *rgb_data;
unsigned char *rgba_data;
volatile int g_keepRunning = 1;
void *npu_output[100];
aiw_u64_t npu_buffer_size[100];

#ifdef USE_POSTPROCESS
static cairo_t* cr;
static cairo_surface_t* cr_surface;
cnn_output_info cnn_output[100];

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} structRGB;

typedef struct {
    int max_class_cnt;
    char** class_names;
    structRGB* class_colors;
} structObjDrawInfo;

float YOLOV5S_CONFIDENCE_OBJECTNESS_THRESHOLD = 0;
float YOLOV5S_CONFIDENCE_DETECTION_THRESHOLD = 0;
float YOLOV5S_CONFIDENCE_NMS_THRESHOLD = 0;

int YOLOV5S_MAX_CLASS_ID_CNT = 0;
int YOLOV5S_ANCHORS_NUM = 0;
char** yolov5s_class_names = NULL;
float* yolov5s_anchors = NULL;
uint16_t* yolov5s_class_id = NULL;
#endif

/*
********************************************************************************
*               FUNCTION DEFINITIONS
********************************************************************************
*/
void sig_int_handler(void)
{
    g_keepRunning = 0;
}

#ifdef USE_POSTPROCESS
void cairo_init(unsigned char *rgb, unsigned int w, unsigned int h)
{
    cr_surface = cairo_image_surface_create_for_data((unsigned char*)rgb,
                CAIRO_FORMAT_ARGB32, w, h, cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, w));
    cr = cairo_create(cr_surface);
}

void cairo_destroy(void)
{
    if(cr)
        cairo_destroy(cr);
    if(cr_surface)
        cairo_surface_destroy (cr_surface);
}

void nc_cairo_draw_object_detections (cairo_t *canvas, structObjDrawInfo* drawInfo, stCnnPostprocessingResults *cnn_results)
{
    static int canvas_w = 0;
    static int canvas_h = 0;
    static float ratio = 1.f;
    if (canvas_w == 0 || canvas_h == 0) {
        cairo_surface_t* cr_surface = cairo_get_target(canvas);
        canvas_w = cairo_image_surface_get_width(cr_surface);
        canvas_h = cairo_image_surface_get_height(cr_surface);
        ratio = (float)canvas_w/1280;
    }

    char text[128] = {'\0',};
    cairo_select_font_face(canvas, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(canvas, 21*ratio);
    cairo_set_line_width(canvas, 2.0*ratio);

    for (int i = 0; i < drawInfo->max_class_cnt; i++) {
        for (int bidx = 0; bidx < cnn_results->class_objs[i].obj_cnt; bidx++) {
            stObjInfo obj_info = cnn_results->class_objs[i].objs[bidx];
            if (obj_info.bbox.w >= 3 && obj_info.bbox.h >= 3)
            {
            #ifdef SHOW_CNN_PROBABILITY
                // show cnn probability (not track id)
                if (obj_info.track_id < 0) sprintf((char *)text, "%s:%0.2f", drawInfo->class_names[i], obj_info.prob);
                else sprintf((char *)text, "%s:%0.2f", drawInfo->class_names[i], obj_info.prob);
            #else
                // show track id (not cnn probability)
                if (obj_info.track_id < 0) sprintf((char *)text, "%s", drawInfo->class_names[i]);
                else sprintf((char *)text, "%s:%d", drawInfo->class_names[i], obj_info.track_id);
            #endif

                cairo_set_source_rgb (canvas, drawInfo->class_colors[i].r, drawInfo->class_colors[i].g, drawInfo->class_colors[i].b);
                cairo_rectangle(canvas, obj_info.bbox.x, obj_info.bbox.y, obj_info.bbox.w, obj_info.bbox.h);

                float lx, ly, lw, lh;
                cairo_text_extents_t c_ext;
                cairo_text_extents(canvas, text, &c_ext);

    #ifdef SHOW_CLASS_LABEL_BOX
                cairo_stroke (canvas);
                lh = (float)c_ext.height+4;
                lw = (float)c_ext.width+4;

                lx = obj_info.bbox.x-ratio;
                ly = obj_info.bbox.y-lh;

                cairo_rectangle(canvas, lx, ly, lw, lh);
                cairo_fill (canvas);

                cairo_set_source_rgb (canvas, 1.0-drawInfo->class_colors[i].r, 1.0-drawInfo->class_colors[i].g, 1.0-drawInfo->class_colors[i].b);
                cairo_move_to(canvas, lx+1, ly + c_ext.height+1);
                cairo_show_text(canvas, (char *)text);
                cairo_stroke (canvas);
    #else
                lx = (obj_info.bbox.x+obj_info.bbox.w/2)-(c_ext.width/2);
                ly = (obj_info.bbox.y+obj_info.bbox.h/2)-(c_ext.height/2);
                cairo_move_to(canvas, lx, ly);
                cairo_show_text(canvas, (char *)text);
                cairo_stroke (canvas);
    #endif
            }
        }
    }
}
#endif

#ifdef SAVE_META_DATA
int make_detfile(detection *dets, int cnt, int file_cnt, int total_file_cnt, char *result_folder_name)
{
    FILE *fp;
    frame_info_t frame;

    memset(&frame, 0, sizeof(frame));

    for(int i=0; i< cnt; i++)
    {
        int pred_class = 0;
        float best_score = 0;
        int class_index = 0;
        for (class_index = 0; class_index < MAX_CLASS_ID_CNT; class_index++) {
            float curr_score = dets[i].prob[class_index];
            if (curr_score > best_score) {
                best_score = curr_score;
                pred_class = class_index;
            }
        }
        int box_x = (int)(dets[i].bbox.x - dets[i].bbox.w / 2);
        int box_y = (int)(dets[i].bbox.y - dets[i].bbox.h / 2);
        int box_width = (int)(dets[i].bbox.w);
        int box_height =(int)(dets[i].bbox.h);

        if(best_score <= 0)
            continue;

        frame.chn[pred_class].box[frame.chn[pred_class].box_cnt].x = box_x;
        frame.chn[pred_class].box[frame.chn[pred_class].box_cnt].y = box_y;
        frame.chn[pred_class].box[frame.chn[pred_class].box_cnt].w = box_width;
        frame.chn[pred_class].box[frame.chn[pred_class].box_cnt].h = box_height;
        frame.chn[pred_class].box[frame.chn[pred_class].box_cnt].s = best_score;
        frame.chn[pred_class].box_cnt++;
    }
    frame.cnt = file_cnt;
    frame.total_file_cnt = total_file_cnt;
    //printf("make json file\n");
    //make_json(&frame, result_folder_name);
    make_json_separate(&frame, result_folder_name);
    return 0;
}

static void app_status_file_start(void)
{
    FILE *fp;
    char status[256] = {"start"};

    fp = fopen(STATUS_FILE_NAME, "wb");
    if (!fp)
    {
        printf("can't open %s\n", STATUS_FILE_NAME);
        exit(0);
    }
    fwrite(status, 1, strlen(status), fp);
    fclose(fp);
}

static void app_status_file_end(void)
{
    FILE *fp;
    char status[256] = {"end  "};

    fp = fopen(STATUS_FILE_NAME, "wb");
    if (!fp)
    {
        printf("can't open %s\n", STATUS_FILE_NAME);
        exit(0);
    }
    fwrite(status, 1, strlen(status), fp);
    fclose(fp);
}
#endif

#ifdef DUMP_CNN_OUTPUT
static void dump_network_output(char *filename, int size, void *npu_output)
{
    FILE *fp;

    fp = fopen(filename, "wb");
    if (!fp)
    {
        printf("can't open %s\n", filename);
        exit(0);
    }
    fwrite(npu_output, 1, size, fp);
    fclose(fp);
}
#endif

char* getExt(char* filename)
{
    static char buf[MAX_PATH] = "";
    char* ptr = NULL;

    ptr = strrchr(filename, '.');
    if (ptr == NULL)
        return NULL;

    strcpy(buf, ptr + 1);

    return buf;
}

int get_filecount(const char* filename)
{
    FILE* fd;
    char line[200];
    int file_cnt=0;

    fd = fopen(filename, "r");
    if (!fd) {
        printf("failed to open file(%s)\n", filename);
        return -1;
    }

    while (fgets(line, sizeof(line), fd) != NULL ) {
        //printf("%s", line);
        file_cnt++;
    }

    printf("image file count = %d\n", file_cnt);
    fclose(fd);
    return file_cnt;
}

void RGBInterleaved2Plane(unsigned char *rgbdata, int width, int height, unsigned char *r, unsigned char *g, unsigned char *b)
{
    for (int y=0; y<height; y++)
    {
        for (int x=0; x<width; x++)
        {
            r[y*width + x] = rgbdata[(y*width + x)*3 + 0];
            g[y*width + x] = rgbdata[(y*width + x)*3 + 1];
            b[y*width + x] = rgbdata[(y*width + x)*3 + 2];
        }
    }
}

aiwDevice* openDevice()
{
    // If real device support is available on the current platform, then try to open one physical device.
    // aiwDeviceCount returns the number of physical devices presented in the current system.
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

    return device;
}

void load_aiwbin(const char* binary_path)
{
    aiwModuleVersion aiwModuleVerInfo;
    aiw_u32_t t_count = 0;
    aiwTensor *tensor = NULL;
    const aiwTensorInfo *tinfo = NULL;

    aiwRuntimeVerInfo = aiwRuntimeVersion();
    printf("aiWare Runtime version:  %s\n", aiwRuntimeVerInfo.version);
    aiwModuleVerInfo = aiwCommonVersion();
    printf("aiwModule Version: %s\n", aiwModuleVerInfo.version);

    printf("Loading binary : %s\n", binary_path);
    binary = aiwBinaryLoadFromPath(binary_path);
    if (!binary)
    {
        fprintf(stderr, "Failed to load binary\n");
        exit(0);
    }

    printf("Opening device\n");
    device = openDevice();
    if (!device)
    {
        fprintf(stderr, "Failed to open device\n");
        exit(0);
    }

    printf("Creating a program\n");
    // Please note, that binary should have been compiled for the same device configuration
    // as the opened device. During program initialization this will be checked, and it the
    // configurations mismatch, then program building fails.
    program = aiwBuildSimpleProgramByCopy(device, binary);
    if (!program)
    {
        fprintf(stderr, "Failed to create program from the binary on the device\n");
        exit(0);
    }

    // check input resolution
    t_count = aiwProgramGetInputTensorCount(program);
    printf("\ninput_tensor_count = %d\n", t_count);
    for (aiw_u32_t i = 0; i < t_count; ++i)
    {
        // The following object must not be deleted, they are owned by the program.
        // How acquired buffers must be released when they are not used anymore.

        // Get the input tensor
        tensor = aiwProgramGetInputTensor(program, i);
        if (!tensor)
        {
            fprintf(stderr, "Invalid input tensor object\n");
            exit(0);
        }

        // Get the corresponding information object
        tinfo = aiwTensorGetInfo(tensor);
        if (!tinfo)
        {
            fprintf(stderr, "Failed to get info for tensor\n");
            exit(0);
        }

        network_input_width = tinfo->dim.w;
        network_input_height = tinfo->dim.h;
        network_input_chanel = tinfo->dim.ch;
        network_input_chanel = tinfo->dim.b;
        printf("input_tensor_dimention_width = %d\n", tinfo->dim.w);
        printf("input_tensor_dimention_height = %d\n", tinfo->dim.h);
        printf("input_tensor_dimention_chanel = %d\n", tinfo->dim.ch);
        printf("input_tensor_dimention_batch = %d\n", tinfo->dim.b);
    }

    // check output resolution
    t_count = aiwProgramGetOutputTensorCount(program);
    printf("\noutput_tensor_count = %d\n", t_count);
    for (aiw_u32_t i = 0; i < t_count; ++i)
    {
        tensor = aiwProgramGetOutputTensor(program, i);
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

        network_output_width = tinfo->dim.w;
        network_output_height = tinfo->dim.h;
        network_output_chanel = tinfo->dim.ch;
        network_output_batch = tinfo->dim.b;
        printf("output_tensor_dimention_width = %d\n", tinfo->dim.w);
        printf("output_tensor_dimention_height = %d\n", tinfo->dim.h);
        printf("output_tensor_dimention_chanel = %d\n", tinfo->dim.ch);
        printf("output_tensor_dimention_batch = %d\n\n", tinfo->dim.b);
    }

}

void run_npu(unsigned char *tiled_data)
{
    aiwTensor *tensor = NULL;
    void *inference_output = NULL;
    const aiwTensorInfo *tinfo = NULL;
    static aiw_u64_t buffer_size = 0;
    aiw_status status = AIW_ERROR;
    uint64_t start_time;
    uint64_t elapsed_ms = 0;

    tensor_count = aiwProgramGetInputTensorCount(program);
    for (aiw_u32_t i = 0; i < tensor_count; ++i)
    {
        // Get the input tensor
        tensor = aiwProgramGetInputTensor(program, i);
        if (!tensor)
        {
            fprintf(stderr, "Invalid input tensor object\n");
            exit(0);
        }
    }

    status = aiwTensorSetDataNCHWUInt8(tensor, tiled_data);
    if (status != AIW_SUCCESS)
    {
        fprintf(stderr, "Failed to initialize input tensor with random data\n");
        exit(0);
    }

    // Upload the input tensors' content from the host memory into aiWare's memory.
    // Please note that this step is always required, even when the host and aiWare share
    // the same memory. In that case CPU cache is flushed to make sure the latest data will
    // be seen by aiWare.
    status = aiwProgramUploadInputs(program);
    if (status != AIW_SUCCESS)
    {
        fprintf(stderr, "Failed to upload input into aiWare's memory\n");
        exit(0);
    }

    //printf("execute network\n");
    start_time = nc_get_mono_us_time();
    status = aiwProgramExecute(program);
    elapsed_ms = nc_elapsed_us_time(start_time);
    printf("cnn_inference_tiem time : %lu us\n", elapsed_ms);
    if (status != AIW_SUCCESS)
    {
        fprintf(stderr, "Failed to execute program\n");
        exit(0);
    }

    //printf("download output\n");
    status = aiwProgramDownloadOutputs(program);
    if (status != AIW_SUCCESS)
    {
        fprintf(stderr, "Failed to download output from aiWare's memory\n");
        exit(0);
    }

    //printf("Print output tensor info and save the tensors into files\n");
    tensor_count = aiwProgramGetOutputTensorCount(program);

    for (aiw_u32_t i = 0; i < tensor_count; ++i)
    {
        tensor = aiwProgramGetOutputTensor(program, i);
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

        buffer_size = aiwTensorSizeNCHW(tensor);

        if (tinfo->exponent) { // data type : float
            buffer_size = buffer_size * sizeof(aiw_f32_t);
            inference_output = (aiw_f32_t*)malloc(buffer_size);
            if (!inference_output)
            {
                fprintf(stderr, "Failed to allocate output float buffer\n");
                exit(0);
            }
            memset(inference_output, 0, buffer_size);

            if (aiwTensorGetDataNCHWFloat(tensor, (aiw_f32_t *)inference_output) != AIW_SUCCESS)
            {
                fprintf(stderr, "Failed to get output float tensor data\n");
                exit(0);
            }
        } else { // data type : uint8_t
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
        //printf("npu_buffer_size = %d\n", buffer_size);
        npu_buffer_size[i] = buffer_size;
        npu_output[i] = (unsigned char *)malloc(buffer_size);
        if(!npu_output)
        {
            printf("ERROR: Failed to allocate buffer for NPU output\n");
        }
        memcpy(npu_output[i], inference_output, buffer_size);

#ifdef USE_POSTPROCESS
        cnn_output[i].total_tensor_cnt = tensor_count;
        cnn_output[i].index_of_total = (uint8_t)i;
        cnn_output[i].cam_ch = 0;
        cnn_output[i].width = tinfo->dim.w;
        cnn_output[i].height = tinfo->dim.h;
        cnn_output[i].channel = (uint8_t)tinfo->dim.ch;
        cnn_output[i].tinfo_in.dim.h = network_input_height;
        cnn_output[i].tinfo_in.dim.w = network_input_width;
        cnn_output[i].data_type = tinfo->exponent?CNN_TILED_DT_FLOAT:CNN_TILED_DT_U8;
        cnn_output[i].tinfo = (aiwTensorInfo *)tinfo;
        cnn_output[i].buffer_size = buffer_size;
        cnn_output[i].tiled_data = (void *)malloc(buffer_size);
        if(!cnn_output[i].tiled_data)
        {
            printf("ERROR: Failed to allocate buffer for tiled_data\n");
        }
        memcpy(cnn_output[i].tiled_data, inference_output, buffer_size);
#endif
        free(inference_output);
        tensor = NULL;
    }
    // elapsed_ms = nc_elapsed_us_time(start_time);
    // printf("cnn_inference_tiem time : %llu us\n", elapsed_ms);
}

void usage(unsigned char *prog)
{
    printf("Usage: %s <path_of_aiwbin> <path_of_image_list_txt> <path_of_cnn_output_result_folder>\n", prog);
    printf("ex) %s misc/networks/Yolov5s/yolov5s_A6_640x384_7class.aiwbin image_list_txt result/yolov5s_cnn_output\n", prog);

    exit(0);
}

int main( int argc, char ** argv)
{
    char fname[255] = {0,};
    int total_file_cnt=0;
    int file_cnt=0;
    char line[200];
    void *rtn;
    FILE* fd;
    char img_fname[255] = {0,};
    char cnn_result_fname[255] = {0,};
    char *img_list_fname;
    unsigned char *npu_input_img_buff;
    unsigned char *r, *g, *b;
#ifdef DUMP_CNN_OUTPUT
    aiw_u64_t total_npu_buffer_size;
    unsigned int npu_addr_offset = 0;
    unsigned char *npu_result_buff;
    char binfilename[255] = {0,};
#endif
    char imgfilename[255] = {0,};

    if (argc != 4)
        usage((unsigned char *)argv[0]);

    nc_init_path_localizer();

    //printf("%s::%d load aiwbin\n", __FILE__, __LINE__);
    load_aiwbin((const char *)argv[1]);

#ifdef USE_POSTPROCESS
    if (nc_set_network_info((const char *)argv[1], NETWORK_YOLOV5_DET) < 0){
        printf("Failed to set network info\n");
        return -1;
    }
#endif

    //printf("%s::%d fname\n", __FILE__, __LINE__);
    memcpy(fname ,argv[2], strlen(argv[2]));
    printf("\nimage list file= %s\n", fname);
    if(strlen(fname) != 0) {
        printf("image list =%s\n", fname);
        img_list_fname = fname;
        total_file_cnt = get_filecount(img_list_fname);
    }
    else {
        printf("Invalid name of image list file %s\n", fname);
        exit(0);
    }

    //printf("%s::%d cnn_result_fname\n", __FILE__, __LINE__);
    memcpy(cnn_result_fname ,argv[3], strlen(argv[3]));
    if(strlen(cnn_result_fname) == 0)
    {
        printf("Invalid name of cnn result file %s\n", cnn_result_fname);
        exit(0);
    }
    printf("cnn result file= %s\n", cnn_result_fname);

    if(total_file_cnt <= 0)
    {
        printf("failed to get file count %s\n", img_list_fname);
        exit(0);
    }
    fd = fopen(img_list_fname, "rt");
    if (!fd) {
        printf("failed to open image list file(%s)\n", img_list_fname);
        exit(0);
    }

    while(g_keepRunning)
    {
        /******** image read ********/
        rtn = fgets(line, sizeof(line), fd);
        if(rtn == NULL)
        {
            printf("==== End of file ====\n");
            g_keepRunning =0;
            exit(0);
        }
#ifdef SAVE_META_DATA
        app_status_file_start();
#endif
        printf("\n%d. ", file_cnt);
        line[strlen(line) - 1] = '\0';
        sprintf(img_fname, "%s", line);
        img_fname[strlen(img_fname) - 1] = '\0';
        printf("img_fname = %s\n", img_fname);

        cv::Mat img_bgr = cv::imread(nc_localize_path((const char*)img_fname));
        if(img_bgr.empty())
        {
            printf("### image load failed (%s)\n", nc_localize_path((const char*)img_fname));
        }

        rgb_data = (unsigned char *)malloc(img_bgr.cols*img_bgr.rows*RGB_CNT);
        if(!rgb_data)
        {
            printf("ERROR: Failed to allocate buffer for rgb_data\n");
        }

        /******** convert BGR to RGB ********/
        cv::Size szSize(img_bgr.rows, img_bgr.cols);
        cv::Mat img_rgb(szSize, CV_8UC3, rgb_data);
        cvtColor(img_bgr, img_rgb, cv::COLOR_BGR2RGB);
        memcpy(rgb_data, img_rgb.data, img_rgb.cols*img_rgb.rows*RGB_CNT);

        npu_input_img_buff = (unsigned char *)malloc(network_input_width * network_input_height * RGB_CNT);
        if(!npu_input_img_buff)
        {
            printf("ERROR: Failed to allocate buffer for NPU input image\n");
        }

        /******** packed RGB to planar RGB ********/
        r = npu_input_img_buff;
        g = r + (network_input_width * network_input_height);
        b = r + ((network_input_width * network_input_height)*2);
        RGBInterleaved2Plane(rgb_data, network_input_width, network_input_height, r, g, b);

        /******** run npu ********/
        run_npu((unsigned char *)npu_input_img_buff);

#ifdef DUMP_CNN_OUTPUT
        /******** dump cnn output result by binary ********/
        total_npu_buffer_size = 0;
        for (aiw_u32_t i = 0; i < tensor_count; ++i)
        {
            total_npu_buffer_size += npu_buffer_size[i];
        }
        npu_result_buff = (unsigned char *)malloc(total_npu_buffer_size);
        for (aiw_u32_t i = 0; i < tensor_count; ++i)
        {
            memcpy(npu_result_buff + npu_addr_offset, npu_output[i], npu_buffer_size[i]);
            npu_addr_offset += npu_buffer_size[i];
        }
        printf("Dump cnn_output_%d.bin\n", file_cnt);
        sprintf(binfilename, "%s_%d.bin", cnn_result_fname, file_cnt);
        dump_network_output(binfilename, total_npu_buffer_size, npu_result_buff);
        free(npu_result_buff);
#endif

#ifdef USE_POSTPROCESS
        /******** use postprocess ********/
        static detection dets[BOX_MAX_NUM] = {{{0,0,0,0}, {0,}, 0},};
        int det_num = 0;
        stNetwork_info* net_info = NULL;

        net_info = nc_cnn_get_network_info(NETWORK_YOLOV5_DET);
        if (net_info == NULL){
            printf("%s, nc_cnn_get_network_info fail. net_id:%d\n", __FUNCTION__, (int)NETWORK_YOLOV5_DET);
            exit(0);
        }

        YOLOV5S_ANCHORS_NUM = (int)net_info->anchor_num;
        YOLOV5S_MAX_CLASS_ID_CNT = (int)net_info->class_num;
        yolov5s_class_names = (char**)net_info->class_name;
        yolov5s_anchors = (float *)net_info->anchor_size;
        yolov5s_class_id = (uint16_t *)net_info->class_id;
        YOLOV5S_CONFIDENCE_OBJECTNESS_THRESHOLD = (float)net_info->obj_th;
        YOLOV5S_CONFIDENCE_DETECTION_THRESHOLD = (float)net_info->det_th;
        YOLOV5S_CONFIDENCE_NMS_THRESHOLD = (float)net_info->nms_th;

        stCnnPostprocessingResults yolov5s_detect_results;

        static structObjDrawInfo drawInfo;
        drawInfo.max_class_cnt = (int)YOLOV5S_MAX_CLASS_ID_CNT;
        drawInfo.class_names = (char**)yolov5s_class_names;
        drawInfo.class_colors = (structRGB*)net_info->class_color;

        for (aiw_u32_t t_cnt = 0; t_cnt < tensor_count; t_cnt++)
        {
            int output_idx = cnn_output[t_cnt].index_of_total;
            if (output_idx == 0) {
                memset(dets, 0, sizeof(dets));
                det_num = 0;
            }

            unsigned int output_w = cnn_output[t_cnt].width;
            unsigned int output_h = cnn_output[t_cnt].height;
            float *output_tiled_data = (float*)cnn_output[t_cnt].tiled_data;

            int side_y,side_x,obj_index = 0;

            int gridside_width = output_w;
            int gridside_height = output_h;

            int image_height = cnn_output[t_cnt].tinfo_in.dim.h;
            int image_width = cnn_output[t_cnt].tinfo_in.dim.w;

            int cell_size_x = image_width / gridside_width;
            int cell_size_y = image_height / gridside_height;

            int boxes_num = 3;//YOLOV5S_ANCHORS_NUM;
            int anchor_num = YOLOV5S_ANCHORS_NUM;

            const int screen_size = gridside_width * gridside_height;
            const int box_size = (5 + YOLOV5S_MAX_CLASS_ID_CNT) * screen_size;

            for (side_y = 0; side_y < gridside_height; ++side_y) {
                for (side_x = 0; side_x < gridside_width; ++side_x) {
                    for (obj_index = 0; obj_index < boxes_num; ++obj_index)
                    {
                        int box_index = side_y * gridside_width + side_x + obj_index * box_size;
                        float confidence = output_tiled_data[box_index + 4 * screen_size];
                        confidence = nc_Logistic_activate(confidence);
                        dets[det_num].objectness = (confidence > YOLOV5S_CONFIDENCE_OBJECTNESS_THRESHOLD) ? confidence : 0;

                        if (dets[det_num].objectness>0)
                        {
                            float x = (float)((nc_Logistic_activate(output_tiled_data[box_index + 0 * screen_size])*2 - 0.5 + side_x) * cell_size_x);
                            float y = (float)((nc_Logistic_activate(output_tiled_data[box_index + 1 * screen_size])*2 - 0.5 + side_y) * cell_size_y);
                            float w = (float)(yolov5s_anchors[output_idx*boxes_num+obj_index] * pow(2*nc_Logistic_activate(output_tiled_data[box_index + 2 * screen_size]),2));
                            float h = (float)(yolov5s_anchors[output_idx*boxes_num+obj_index+anchor_num] * pow(2*nc_Logistic_activate(output_tiled_data[box_index + 3 * screen_size]),2));

                            stBBox b;
                            b.x = x * (float)network_input_width / (float)image_width ;
                            b.y = y * (float)network_input_height / (float)image_height;
                            b.w = w * (float)network_input_width / (float)image_width ;
                            b.h = h * (float)network_input_height / (float)image_height;

                            dets[det_num].bbox = b;

                            for (int class_index = 0; class_index < YOLOV5S_MAX_CLASS_ID_CNT; ++class_index) {
                                float curr_score_pre = (float)(output_tiled_data[box_index + (5 + class_index) * screen_size]);
                                float curr_score = nc_Logistic_activate(curr_score_pre);
                                dets[det_num].prob[class_index] = (curr_score > YOLOV5S_CONFIDENCE_DETECTION_THRESHOLD) ? curr_score : 0;
                            }

                            (det_num)++;
                            if (det_num >= BOX_MAX_NUM)
                            {
                                break;
                            }
                        }
                    }
                }
            }


            if((uint32_t)cnn_output[t_cnt].index_of_total+1 == cnn_output[t_cnt].total_tensor_cnt)
            {
                nc_Nms_box(dets, &det_num, YOLOV5S_MAX_CLASS_ID_CNT, YOLOV5S_CONFIDENCE_NMS_THRESHOLD);
#ifdef SAVE_META_DATA
                /******** make json file ********/
                make_detfile(dets, det_num, file_cnt, total_file_cnt, cnn_result_fname);
                app_status_file_end();
                //nc_Nms_box(dets, &det_num, YOLOV5S_MAX_CLASS_ID_CNT, YOLOV5S_CONFIDENCE_NMS_THRESHOLD);
#endif
                memset(&yolov5s_detect_results, 0, sizeof(stCnnPostprocessingResults));
                for(int i = 0; i < det_num; i++)
                {
                    int pred_class = 0;
                    float best_score = 0;
                    for (int class_index = 0; class_index < YOLOV5S_MAX_CLASS_ID_CNT; class_index++) {
                        float curr_score = dets[i].prob[class_index];
                        if (curr_score > best_score) {
                            best_score = curr_score;
                            pred_class = class_index;
                        }
                    }

                    float box_x = (dets[i].bbox.x - dets[i].bbox.w / 2);
                    float box_y = (dets[i].bbox.y - dets[i].bbox.h / 2);
                    float box_w = (dets[i].bbox.w);
                    float box_h = (dets[i].bbox.h);

                    for (int class_index = 0; class_index < YOLOV5S_MAX_CLASS_ID_CNT; class_index++)
                    {
                        if ((pred_class == class_index) & (dets[i].objectness*best_score >= YOLOV5S_CONFIDENCE_DETECTION_THRESHOLD))
                        {

                                int idx = yolov5s_detect_results.class_objs[pred_class].obj_cnt;
                                if (idx == MAX_CNN_RESULT_CNT_OF_CLASS) {
                                    continue;
                                }
                                yolov5s_detect_results.class_objs[pred_class].class_id = pred_class;
                                yolov5s_detect_results.class_objs[pred_class].objs[idx].track_id = -1;
                                yolov5s_detect_results.class_objs[pred_class].objs[idx].prob = dets[i].objectness*best_score;
                                yolov5s_detect_results.class_objs[pred_class].objs[idx].bbox.x = box_x;
                                yolov5s_detect_results.class_objs[pred_class].objs[idx].bbox.y = box_y;
                                yolov5s_detect_results.class_objs[pred_class].objs[idx].bbox.w = box_w;
                                yolov5s_detect_results.class_objs[pred_class].objs[idx].bbox.h = box_h;
                                yolov5s_detect_results.class_objs[pred_class].obj_cnt ++;
                        }
                    }
                }
            }
        }
#endif

#ifdef SAVE_OUTPUT_RESULT_IMAGE
        /******** convert RGB to RGBA ********/
        rgba_data = (unsigned char *)malloc(img_rgb.cols*img_rgb.rows*RGBA_CNT);
        if(!rgba_data)
        {
            printf("ERROR: Failed to allocate buffer for rgb_data\n");
        }
        cv::Mat img_rgba(szSize, CV_8UC4, rgba_data);
        cvtColor(img_rgb, img_rgba, cv::COLOR_RGB2RGBA);
        memcpy(rgba_data, img_rgba.data, img_rgba.cols*img_rgba.rows*RGBA_CNT);

        /******** draw postprocess result into image ********/
        cairo_init(rgba_data, network_input_width, network_input_height);
        if(cr) {
            nc_cairo_draw_object_detections(cr, &drawInfo, &yolov5s_detect_results);
        }

        /******** convert RGBA to BGRA ********/
        cv::Mat draw_rgba_img(network_input_height, network_input_width, CV_8UC4);
        memcpy(draw_rgba_img.data, rgba_data, network_input_width*network_input_height*RGBA_CNT);
        cv::Mat draw_bgra_img(network_input_height, network_input_width, CV_8UC4);
        cvtColor(draw_rgba_img, draw_bgra_img, cv::COLOR_RGBA2BGRA);

        /******** save image for check cnn output result ********/
        printf("save image for check cnn output result - %d\n",file_cnt);
        sprintf(imgfilename, "%s_%d.png", cnn_result_fname, file_cnt);
        cv::imwrite(imgfilename, draw_bgra_img);

        cairo_destroy();
        free(rgba_data);
#endif

        file_cnt++;
#ifdef DUMP_CNN_OUTPUT
        npu_addr_offset = 0;
#endif
        for (aiw_u32_t i = 0; i < tensor_count; ++i)
        {
            free(npu_output[i]);
#ifdef USE_POSTPROCESS
            free(cnn_output[i].tiled_data);
#endif
        }
        free(npu_input_img_buff);
        free(rgb_data);
    }

    printf("npu_app is terminated\n");

    return 0;
}
