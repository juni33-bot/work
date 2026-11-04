#include "opencv2/opencv.hpp"
#include "nc_opencv_wrapper.h"
#include "nc_utils.h"
using namespace std;
using namespace cv;

int nc_opencv_resize(struct img_info *src, struct img_info *dst)
{
    Mat dst_img(dst->height, dst->width, CV_8UC4, dst->buff);
    Mat src_img(src->height, src->width, CV_8UC4, src->buff);
    int resized_size;
    int res = 0;

    //uint64_t start_time = nc_get_mono_time();
    resize(src_img, dst_img, Size(dst->width, dst->height), INTER_LINEAR);
    //printf("resize_elapse_time: %lld\n", nc_elapsed_time(start_time));
    resized_size = dst_img.cols * dst_img.rows * dst_img.channels();
    //printf("<%s> %d \n", __func__, resized_size);
    return resized_size;
}

int nc_opencv_rgbplanar_resize(struct img_info *src, struct img_info *dst)
{
    uchar *src_buff_ptr;
    uchar *dst_buff_ptr;
    int res = dst->width*dst->height*2;

    src_buff_ptr = src->buff;
    dst_buff_ptr = dst->buff;
    Mat r_in(src->height, src->width, CV_8UC1, src_buff_ptr);
    Mat r_out(dst->height, dst->width, CV_8UC1, dst_buff_ptr);
    resize(r_in, r_out, Size(dst->width, dst->height), INTER_LINEAR);

    src_buff_ptr += src->width*src->height;
    dst_buff_ptr += dst->width*dst->height;
    Mat g_in(src->height, src->width, CV_8UC1, src_buff_ptr);
    Mat g_out(dst->height, dst->width, CV_8UC1, dst_buff_ptr);
    resize(g_in, g_out, Size(dst->width, dst->height), INTER_LINEAR);

    src_buff_ptr += src->width*src->height;
    dst_buff_ptr += dst->width*dst->height;
    Mat b_in(src->height, src->width, CV_8UC1, src_buff_ptr);
    Mat b_out(dst->height, dst->width, CV_8UC1, dst_buff_ptr);
    resize(b_in, b_out, Size(dst->width, dst->height), INTER_LINEAR);

    return res;
}

/*
    only for yuv422 planar, not packed format
    example) scale up 2M to 5M

    in_yuv_buff = (uint8_t *)malloc(1920*1080*2);
    out_yuv_buff = (uint8_t *)malloc(2880*1860*2);

    src_img.width = 1920;
    src_img.height = 1080;
    src_img.buff = in_yuv_buff;

    dst_img.width = 2880;
    dst_img.height = 1860;
    dst_img.buff = out_yuv_buff;
    nc_opencv_yuv422p_resize(&src_img, &dst_img);
*/
int nc_opencv_yuv422p_resize(struct img_info *src, struct img_info *dst)
{
    uchar *src_buff_ptr;
    uchar *dst_buff_ptr;
    int res = dst->width*dst->height*2;

    src_buff_ptr = src->buff;
    dst_buff_ptr = dst->buff;
    Mat y_in(src->height, src->width, CV_8UC1, src_buff_ptr);
    Mat y_out(dst->height, dst->width, CV_8UC1, dst_buff_ptr);
    resize(y_in, y_out, Size(dst->width, dst->height), INTER_LINEAR);

    src_buff_ptr += src->width*src->height;
    dst_buff_ptr += dst->width*dst->height;
    Mat u_in(src->height/2, src->width, CV_8UC1, src_buff_ptr);
    Mat u_out(dst->height/2, dst->width, CV_8UC1, dst_buff_ptr);
    resize(u_in, u_out, Size(dst->width, dst->height/2), INTER_LINEAR);

    src_buff_ptr += src->width*src->height/2;
    dst_buff_ptr += dst->width*dst->height/2;
    Mat v_in(src->height/2, src->width, CV_8UC1, src_buff_ptr);
    Mat v_out(dst->height/2, dst->width, CV_8UC1, dst_buff_ptr);
    resize(v_in, v_out, Size(dst->width, dst->height/2), INTER_LINEAR);

    return res;
}

int nc_opencv_yuv422_to_rgb(struct img_info *yuv, struct img_info *rgb)
{
    int ret = CV_StsOk;    

    Size szSize(rgb->width, rgb->height);
    Mat img_yuv422(szSize, CV_8UC2, yuv->buff);
    Mat img_rgb(szSize, CV_8UC3, rgb->buff);

    cvtColor(img_yuv422, img_rgb, COLOR_YUV2RGB_YUY2);

    return ret;
}

int nc_opencv_rgb_to_yuv420(struct img_info *rgb, struct img_info *yuv)
{
    int ret = CV_StsOk;  

    Size szSize(rgb->width, rgb->height);
    Mat img_yuv420(szSize, CV_8UC2, yuv->buff);
    Mat img_rgb(szSize, CV_8UC3, rgb->buff);
    
    cvtColor(img_rgb, img_yuv420, COLOR_RGB2YUV_I420);

    return ret;
}

int nc_opencv_rgb_to_yuv444(struct img_info *rgb, struct img_info *yuv)
{
    int ret = CV_StsOk;  

    Size szSize(rgb->width, rgb->height);
    Mat img_yuv444(szSize, CV_8UC3, yuv->buff);
    Mat img_rgb(szSize, CV_8UC3, rgb->buff);

    cvtColor(img_rgb, img_yuv444, COLOR_RGB2YUV);

    return ret;
}
