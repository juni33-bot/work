struct img_info {
    int width;
    int height;
    uint8_t *buff;
};

#ifdef __cplusplus
extern "C" int nc_opencv_resize(struct img_info *src, struct img_info *dst);
extern "C" int nc_opencv_yuv422p_resize(struct img_info *src, struct img_info *dst);
extern "C" int nc_opencv_yuv422_to_rgb(struct img_info *yuv, struct img_info *rgb);
extern "C" int nc_opencv_rgb_to_yuv420(struct img_info *rgb, struct img_info *yuv);
extern "C" int nc_opencv_rgb_to_yuv444(struct img_info *rgb, struct img_info *yuv);
extern "C" int nc_opencv_rgbplanar_resize(struct img_info *src, struct img_info *dst);
#else
extern int nc_opencv_resize(struct img_info *src, struct img_info *dst);
extern int nc_opencv_yuv422p_resize(struct img_info *src, struct img_info *dst);
extern int nc_opencv_yuv422_to_rgb(struct img_info *src, struct img_info *dst);
extern int nc_opencv_rgb_to_yuv420(struct img_info *rgb, struct img_info *yuv);
extern int nc_opencv_rgb_to_yuv444(struct img_info *rgb, struct img_info *yuv);
extern int nc_opencv_rgbplanar_resize(struct img_info *src, struct img_info *dst);
#endif