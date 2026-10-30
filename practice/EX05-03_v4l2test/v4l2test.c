/*
 *  V4L2 video capture example
 *
 *  This program can be used and distributed without restrictions.
 *
 *      This program is provided with the V4L2 API
 * see http://linuxtv.org/docs.php for more information
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h>             /* getopt_long() */

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>
#include <linux/fb.h>

#include <pthread.h>

#define DEBUG
#ifdef DEBUG
	#define debug_print(fmt, args...) {printf(fmt, ##args); fflush(stdout);}
#else
	#define debug_print(arg, ...)
#endif

#define CLEAR(x) memset(&(x), 0, sizeof(x))

#define COLOR_RED ((0x1f<<11)|(0x0<<6)|(0x0<<1))
#define COLOR_GREEN ((0x0<<11)|(0x1f<<6)|(0x0<<1))
#define COLOR_BLUE ((0x0<<11)|(0x0<<6)|(0x1f<<1))
#define COLOR_BLACK ((0x0<<11)|(0x0<<6)|(0x0<<1))
#define COLOR_WHITE ((0x1f<<11)|(0x1f<<6)|(0x1f<<1))

enum io_method {
        IO_METHOD_READ,
        IO_METHOD_MMAP,
        IO_METHOD_USERPTR,
};

struct buffer {
        void   *start;
        size_t  length;
};

static char *dev_name = "/dev/video0";
static enum io_method io = IO_METHOD_MMAP;
static int fd = -1;
static int fd_fb = -1;
char *map_fb;
int size_fb;
struct buffer *buffers;
static unsigned int n_buffers;
static int out_buf;
static char *fb_name = "/dev/fb0";
static int force_format;
static int frame_count = 70;
static struct v4l2_format fmt;
static int xpos, ypos;
static int disp_ratio = 1;
static int file_draw = 0;
static char *file_name;
static int clear_screen = 0;

#define MAX_FILE_BUF (640*2*480)
static char *file_buf;

static void errno_exit(const char *s)
{
        fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
        exit(EXIT_FAILURE);
}

#define MAX_CMD_STR 128
static pthread_t thread_id;
static int capture_flag;
static char cmd_str[MAX_CMD_STR];

static int finish_flag;

void *comm_thread(void *arg)
{
	int ret;

	ret = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	if(ret != 0) errno_exit("pthread_setcancelstate()");
	
	for(;;) {
		fgets(cmd_str, MAX_CMD_STR, stdin);
		/* change '\n' -> '\0' at the last position */
		cmd_str[strlen(cmd_str)-1] = '\0';
		switch(cmd_str[0]) {
			case 'c':
				capture_flag = 1;
				break;
			case 'e':
				finish_flag = 1;
				break;
			default:
				break;
		}
	}
}

static int thread_init(void)
{
	int ret;

	debug_print("[%s:%d->%s()]\n", __FILE__, __LINE__, __FUNCTION__);
	ret = pthread_create(&thread_id, NULL, comm_thread, NULL);
	if(ret != 0) errno_exit("pthread_create()");
	debug_print("[%s:%d->%s()]\n", __FILE__, __LINE__, __FUNCTION__);
	
	return 0;
}

static int thread_finish(void)
{
	int ret;

	ret = pthread_cancel(thread_id);
	if(ret != 0) errno_exit("pthread_cancel()");
	ret = pthread_join(thread_id, NULL);
	if(ret != 0) errno_exit("pthread_join()");

	return 0;
}

static int draw_rect(int x, int y, int w, int h, unsigned int color, struct fb_var_screeninfo *vip, struct fb_fix_screeninfo *fip, char *map)
{
	int xx, yy;
	int location = 0;

	for(yy = y; yy < (y+h); yy++) {
		for(xx = x; xx < (x+w); xx++) {
			location = (xx+vip->xoffset) * (vip->bits_per_pixel/8) +
				(yy+vip->yoffset) * fip->line_length;
			if (vip->bits_per_pixel == 32) {
				*(unsigned int *)(map + location) = color;
			} else  { /* 16bpp */
				*(unsigned short *)(map + location) = (unsigned short)color;
			}
		}
	}

	return 0;
}

static int fb_init(void)
{
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;

	/* open */
	fd_fb = open(fb_name, O_RDWR);
	if(fd_fb == -1) {
		errno_exit("fbdev open");
	}
	/* get fb_var_screeninfo */
	if (ioctl(fd_fb, FBIOGET_VSCREENINFO, &vinfo) == -1) {
		errno_exit("FBIOGET_VSCREENINFO");
	}
	/* get fb_fix_screeninfo */
	if (ioctl(fd_fb, FBIOGET_FSCREENINFO, &finfo) == -1) {
		errno_exit("FBIOGET_FSCREENINFO");
	}
	//printf("%s: %dx%d, %dbpp\n", fb_name, vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);
	/* mmap */
	size_fb = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
	map_fb = (char *)mmap(0, size_fb, PROT_READ | PROT_WRITE, MAP_SHARED, fd_fb, 0);
	if (map_fb == (char *)-1) {
		errno_exit("fbdev mmap");
	}

	if(clear_screen) {
		draw_rect(vinfo.xoffset, vinfo.yoffset, vinfo.xres, vinfo.yres, COLOR_BLACK, &vinfo, &finfo, map_fb);
	}

	return 0;
}

static int fb_close(void)
{
       	if (-1 == close(fd_fb)) {
               	errno_exit("fbdev close");
        }
	fd_fb = -1;
       	if (-1 == munmap(map_fb, size_fb)) {
               	errno_exit("fbdev munmap");
        }

	return 0;
}

int capture_to_file(const void *p, int size, char *file_name)
{
	int ret;
	int fd;
	debug_print("[%s:%d->%s()] size=%d, file_name=%s\n", __FILE__, __LINE__, __FUNCTION__, size, file_name);

	unlink(file_name);
	fd = open(file_name, O_CREAT|O_WRONLY);
	if(fd == -1) {
               	errno_exit("capture_to_file");
		return -1;
	}

	ret = write(fd, p, size);
	if(ret == -1) {
               	errno_exit("capture_to_file");
		return -1;
	}

	fsync(fd);
	close(fd);

	return 0;
}

static int capture_to_fb(const void *p, int size, int x, int y, int ratio)
{
	int i, j;
	unsigned int rgb888;
	unsigned int yuv422;
	int R, G, B;
	unsigned char Y, U, V;
	char *pp = (char *)p;
	int inc = 1 << (ratio-1);
	int width = fmt.fmt.pix.width;
	int height = fmt.fmt.pix.height;

	for(i=0; i<height; i+=inc)
	{
		for(j=0; j<width/2; j+=inc)
		{
			yuv422 = *(unsigned int *)(pp+i*width*2+j*4);

			U = (yuv422>>8)&0xff;
			V = (yuv422>>24)&0xff;


			Y = yuv422&0xff;

			R = Y + 1.4075 * (V - 128);
			if(R>255) R=255;
			if(R<0) R=0;
			G = Y - 3455 * (U - 128)/10000 - (7169 * (V - 128)/10000);
			if(G>255) G=255;
			if(G<0) G=0;
			B = Y + 17790 * (U - 128)/10000;
			if(B>255) B=255;
			if(B<0) B=0;
			if(R<0 || R>255 || G<0 || G>255 || B<0 || B>255) printf("RGB=%d,%d,%d, YUV=%d,%d,%d\n", R, B, B, Y, U, V);

			rgb888 = R<<16 | G<<8 | B;
			//*(unsigned int *)(map_fb+1024*4*(i/inc+y)+((j+x)*2/inc)*4) = rgb888;
			*(unsigned int *)(map_fb+1024*4*i/inc+y*1024*4+(j*2/inc)*4+x*4) = rgb888;

			Y = (yuv422>>16)&0xff;

			R = Y + 1.4075 * (V - 128);
			if(R>255) R=255;
			if(R<0) R=0;
			G = Y - 3455 * (U - 128)/10000 - (7169 * (V - 128)/10000);
			if(G>255) G=255;
			if(G<0) G=0;
			B = Y + 17790 * (U - 128)/10000;
			if(B>255) B=255;
			if(B<0) B=0;
			if(R<0 || R>255 || G<0 || G>255 || B<0 || B>255) printf("RGB=%d,%d,%d, YUV=%d,%d,%d\n", R, B, B, Y, U, V);

			rgb888 = R<<16 | G<<8 | B;
			//*(unsigned int *)(map_fb+1024*4*(i/inc+y)+((j+x)*2/inc+1)*4) = rgb888;
			*(unsigned int *)(map_fb+1024*4*i/inc+y*1024*4+(j*2/inc+1)*4+x*4) = rgb888;
		}
	}

	return 0;
}

static int xioctl(int fh, int request, void *arg)
{
        int r;

        do {
                r = ioctl(fh, request, arg);
        } while (-1 == r && EINTR == errno);

        return r;
}

static void process_image(const void *p, int size)
{
        if (out_buf)
                fwrite(p, size, 1, stdout);

	else if(fb_name) {
		capture_to_fb(p, size, xpos, ypos, disp_ratio);

		if(capture_flag) {
			debug_print("[%s:%d->%s()]\n", __FILE__, __LINE__, __FUNCTION__);
			capture_flag = 0;
			capture_to_file(p, size, cmd_str+2);
			printf("OK\n");
			sleep(1);
		}

	}
}

static int read_frame(void)
{
        struct v4l2_buffer buf;
        unsigned int i;

        switch (io) {
        case IO_METHOD_READ:
                if (-1 == read(fd, buffers[0].start, buffers[0].length)) {
                        switch (errno) {
                        case EAGAIN:
                                return 0;

                        case EIO:
                                /* Could ignore EIO, see spec. */

                                /* fall through */

                        default:
                                errno_exit("read");
                        }
                }

                process_image(buffers[0].start, buffers[0].length);
                break;

        case IO_METHOD_MMAP:
                CLEAR(buf);

                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;

                if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
                        switch (errno) {
                        case EAGAIN:
                                return 0;

                        case EIO:
                                /* Could ignore EIO, see spec. */

                                /* fall through */

                        default:
                                errno_exit("VIDIOC_DQBUF");
                        }
                }

                assert(buf.index < n_buffers);

                process_image(buffers[buf.index].start, buf.bytesused);

                if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                        errno_exit("VIDIOC_QBUF");
                break;

        case IO_METHOD_USERPTR:
                CLEAR(buf);

                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_USERPTR;

                if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
                        switch (errno) {
                        case EAGAIN:
                                return 0;

                        case EIO:
                                /* Could ignore EIO, see spec. */

                                /* fall through */

                        default:
                                errno_exit("VIDIOC_DQBUF");
                        }
                }

                for (i = 0; i < n_buffers; ++i)
                        if (buf.m.userptr == (unsigned long)buffers[i].start
                            && buf.length == buffers[i].length)
                                break;

                assert(i < n_buffers);

                process_image((void *)buf.m.userptr, buf.bytesused);

                if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                        errno_exit("VIDIOC_QBUF");
                break;
        }

        return 1;
}

static void mainloop(void)
{
        unsigned int count;

        count = frame_count;

        while (count-- > 0) {
                for (;;) {
                        fd_set fds;
                        struct timeval tv;
                        int r;

			fflush(stdout);
			if(finish_flag) {
				break;
			}

                        FD_ZERO(&fds);
                        FD_SET(fd, &fds);

                        /* Timeout. */
                        tv.tv_sec = 2;
                        tv.tv_usec = 0;

                        r = select(fd + 1, &fds, NULL, NULL, &tv);

                        if (-1 == r) {
                                if (EINTR == errno)
                                        continue;
                                errno_exit("select");
                        }

                        if (0 == r) {
                                fprintf(stderr, "select timeout\n");
                                exit(EXIT_FAILURE);
                        }

                        if (read_frame())
                                break;
                        /* EAGAIN - continue select loop. */
                }

		if(finish_flag) {
			break;
		}
        }
}

static void stop_capturing(void)
{
        enum v4l2_buf_type type;

        switch (io) {
        case IO_METHOD_READ:
                /* Nothing to do. */
                break;

        case IO_METHOD_MMAP:
        case IO_METHOD_USERPTR:
                type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
                        errno_exit("VIDIOC_STREAMOFF");
                break;
        }
}

static void start_capturing(void)
{
        unsigned int i;
        enum v4l2_buf_type type;

        switch (io) {
        case IO_METHOD_READ:
                /* Nothing to do. */
                break;

        case IO_METHOD_MMAP:
                for (i = 0; i < n_buffers; ++i) {
                        struct v4l2_buffer buf;

                        CLEAR(buf);
                        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        buf.memory = V4L2_MEMORY_MMAP;
                        buf.index = i;

                        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                                errno_exit("VIDIOC_QBUF");
                }
                type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
                        errno_exit("VIDIOC_STREAMON");
                break;

        case IO_METHOD_USERPTR:
                for (i = 0; i < n_buffers; ++i) {
                        struct v4l2_buffer buf;

                        CLEAR(buf);
                        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        buf.memory = V4L2_MEMORY_USERPTR;
                        buf.index = i;
                        buf.m.userptr = (unsigned long)buffers[i].start;
                        buf.length = buffers[i].length;

                        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                                errno_exit("VIDIOC_QBUF");
                }
                type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
                        errno_exit("VIDIOC_STREAMON");
                break;
        }
}

static void uninit_device(void)
{
        unsigned int i;

        switch (io) {
        case IO_METHOD_READ:
                free(buffers[0].start);
                break;

        case IO_METHOD_MMAP:
                for (i = 0; i < n_buffers; ++i)
                        if (-1 == munmap(buffers[i].start, buffers[i].length))
                                errno_exit("munmap");
                break;

        case IO_METHOD_USERPTR:
                for (i = 0; i < n_buffers; ++i)
                        free(buffers[i].start);
                break;
        }

        free(buffers);
}

static void init_read(unsigned int buffer_size)
{
        buffers = calloc(1, sizeof(*buffers));

        if (!buffers) {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }

        buffers[0].length = buffer_size;
        buffers[0].start = malloc(buffer_size);

        if (!buffers[0].start) {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }
}

static void init_mmap(void)
{
        struct v4l2_requestbuffers req;

        CLEAR(req);

        req.count = 4;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;

        if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
                if (EINVAL == errno) {
                        fprintf(stderr, "%s does not support "
                                 "memory mapping\n", dev_name);
                        exit(EXIT_FAILURE);
                } else {
                        errno_exit("VIDIOC_REQBUFS");
                }
        }

        if (req.count < 2) {
                fprintf(stderr, "Insufficient buffer memory on %s\n",
                         dev_name);
                exit(EXIT_FAILURE);
        }

        buffers = calloc(req.count, sizeof(*buffers));

        if (!buffers) {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }

        for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
                struct v4l2_buffer buf;

                CLEAR(buf);

                buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory      = V4L2_MEMORY_MMAP;
                buf.index       = n_buffers;

                if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
                        errno_exit("VIDIOC_QUERYBUF");

                buffers[n_buffers].length = buf.length;
                buffers[n_buffers].start =
                        mmap(NULL /* start anywhere */,
                              buf.length,
                              PROT_READ | PROT_WRITE /* required */,
                              MAP_SHARED /* recommended */,
                              fd, buf.m.offset);

                if (MAP_FAILED == buffers[n_buffers].start)
                        errno_exit("mmap");
        }
}

static void init_userp(unsigned int buffer_size)
{
        struct v4l2_requestbuffers req;

        CLEAR(req);

        req.count  = 4;
        req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_USERPTR;

        if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
                if (EINVAL == errno) {
                        fprintf(stderr, "%s does not support "
                                 "user pointer i/o\n", dev_name);
                        exit(EXIT_FAILURE);
                } else {
                        errno_exit("VIDIOC_REQBUFS");
                }
        }

        buffers = calloc(4, sizeof(*buffers));

        if (!buffers) {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }

        for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
                buffers[n_buffers].length = buffer_size;
                buffers[n_buffers].start = malloc(buffer_size);

                if (!buffers[n_buffers].start) {
                        fprintf(stderr, "Out of memory\n");
                        exit(EXIT_FAILURE);
                }
        }
}

static void init_device(void)
{
        struct v4l2_capability cap;
        struct v4l2_cropcap cropcap;
        struct v4l2_crop crop;

        if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
                if (EINVAL == errno) {
                        fprintf(stderr, "%s is no V4L2 device\n",
                                 dev_name);
                        exit(EXIT_FAILURE);
                } else {
                        errno_exit("VIDIOC_QUERYCAP");
                }
        }

        if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
                fprintf(stderr, "%s is no video capture device\n",
                         dev_name);
                exit(EXIT_FAILURE);
        }

        switch (io) {
        case IO_METHOD_READ:
                if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
                        fprintf(stderr, "%s does not support read i/o\n",
                                 dev_name);
                        exit(EXIT_FAILURE);
                }
                break;

        case IO_METHOD_MMAP:
        case IO_METHOD_USERPTR:
                if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
                        fprintf(stderr, "%s does not support streaming i/o\n",
                                 dev_name);
                        exit(EXIT_FAILURE);
                }
                break;
        }


        /* Select video input, video standard and tune here. */


        CLEAR(cropcap);

        cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
                crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                crop.c = cropcap.defrect; /* reset to default */

                if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
                        switch (errno) {
                        case EINVAL:
                                /* Cropping not supported. */
                                break;
                        default:
                                /* Errors ignored. */
                                break;
                        }
                }
        } else {
                /* Errors ignored. */
        }


        CLEAR(fmt);

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = 640;
	fmt.fmt.pix.height = 480;
	//fmt.fmt.pix.width = 320;
	//fmt.fmt.pix.height = 240;
	fmt.fmt.pix.sizeimage = fmt.fmt.pix.width * fmt.fmt.pix.height * 2;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.field = V4L2_FIELD_NONE;
	if (xioctl(fd, VIDIOC_S_FMT, &fmt) == -1) {
		errno_exit("VIDIOC_S_FMT");
	}

	debug_print("v4l2test: witdh=%d, height=%d, bytesperline=%d, sizeimage=%d, pixelformat %c%c%c%c\n",
			fmt.fmt.pix.width, fmt.fmt.pix.height, fmt.fmt.pix.bytesperline, fmt.fmt.pix.sizeimage,
			fmt.fmt.pix.pixelformat&0xff, (fmt.fmt.pix.pixelformat>>8)&0xff,
			(fmt.fmt.pix.pixelformat>>16)&0xff, (fmt.fmt.pix.pixelformat>>24)&0xff);

        switch (io) {
        case IO_METHOD_READ:
                init_read(fmt.fmt.pix.sizeimage);
                break;

        case IO_METHOD_MMAP:
                init_mmap();
                break;

        case IO_METHOD_USERPTR:
                init_userp(fmt.fmt.pix.sizeimage);
                break;
        }
}

static void close_device(void)
{
        if (-1 == close(fd))
                errno_exit("close");

        fd = -1;
	
	if(fb_name) {
		fb_close();
	}

	thread_finish();
}

static void open_device(void)
{
        struct stat st;

        if (-1 == stat(dev_name, &st)) {
                fprintf(stderr, "Cannot identify '%s': %d, %s\n",
                         dev_name, errno, strerror(errno));
                exit(EXIT_FAILURE);
        }

        if (!S_ISCHR(st.st_mode)) {
                fprintf(stderr, "%s is no device\n", dev_name);
                exit(EXIT_FAILURE);
        }

        fd = open(dev_name, O_RDWR | O_NONBLOCK, 0);

        if (-1 == fd) {
                fprintf(stderr, "Cannot open '%s': %d, %s\n",
                         dev_name, errno, strerror(errno));
                exit(EXIT_FAILURE);
        }

	if(fb_name) {
		fb_init();
	}

	thread_init();
}

static void usage(FILE *fp, int argc, char **argv)
{
        fprintf(fp,
                 "Usage: %s [options]\n"
                 "Options:\n"
                 "-d | --device name   Video device name [%s]\n"
                 "-h | --help          Print this message\n"
                 "-m | --mmap          Use memory mapped buffers [default]\n"
                 "-r | --read          Use read() calls\n"
                 "-u | --userp         Use application allocated buffers\n"
                 "-o | --output        Outputs stream to stdout\n"
                 "-b | --framebuffer   Outputs stream to framebuffer\n"
                 "-f | --format        Force format to specific type\n"
                 "-c | --count         Number of frames to grab [%i]\n"
                 "-x | --xpos position Starting X position to draw [%i]\n"
                 "-y | --ypos position Starting Y position to draw [%i]\n"
                 "-a | --ratio ratio   Reduction ratio to draw [%i]\n"
                 "-n | --file name     Save to file\n"
                 "-s | --clear         Clear screen\n"
                 "",
                 argv[0], dev_name, frame_count, xpos, ypos, disp_ratio);
}

static const char short_options[] = "d:hmruob:fc:x:y:a:n:s";

static const struct option
long_options[] = {
        { "device", required_argument, NULL, 'd' },
        { "help",   no_argument,       NULL, 'h' },
        { "mmap",   no_argument,       NULL, 'm' },
        { "read",   no_argument,       NULL, 'r' },
        { "userp",  no_argument,       NULL, 'u' },
        { "output", no_argument,       NULL, 'o' },
        { "framebuffer", required_argument, NULL, 'b' },
        { "format", no_argument,       NULL, 'f' },
        { "count",  required_argument, NULL, 'c' },
        { "xpos",  required_argument, NULL, 'x' },
        { "ypos",  required_argument, NULL, 'y' },
        { "ratio",  required_argument, NULL, 'a' },
        { "file",  required_argument, NULL, 'n' },
        { "clear", no_argument,       NULL, 's' },
        { 0, 0, 0, 0 }
};

int main(int argc, char **argv)
{
	debug_print("[%s:%d->%s()]\n", __FILE__, __LINE__, __FUNCTION__);

        for (;;) {
                int idx;
                int c;

                c = getopt_long(argc, argv,
                                short_options, long_options, &idx);

                if (-1 == c)
                        break;

                switch (c) {
                case 0: /* getopt_long() flag */
                        break;

                case 'd':
                        dev_name = optarg;
                        break;

                case 'h':
                        usage(stdout, argc, argv);
                        exit(EXIT_SUCCESS);

                case 'm':
                        io = IO_METHOD_MMAP;
                        break;

                case 'r':
                        io = IO_METHOD_READ;
                        break;

                case 'u':
                        io = IO_METHOD_USERPTR;
                        break;

                case 'o':
                        out_buf++;
                        break;

                case 'b':
                        fb_name = optarg;
                        break;

                case 'f':
                        force_format++;
                        break;

                case 'c':
                        errno = 0;
                        frame_count = strtol(optarg, NULL, 0);
                        if (errno)
                                errno_exit(optarg);
                        break;

                case 'x':
                        errno = 0;
                        xpos = strtol(optarg, NULL, 0);
                        if (errno)
                                errno_exit(optarg);
                        break;

                case 'y':
                        errno = 0;
                        ypos = strtol(optarg, NULL, 0);
                        if (errno)
                                errno_exit(optarg);
                        break;

                case 'a':
                        errno = 0;
                        disp_ratio = strtol(optarg, NULL, 0);
                        if (errno)
                                errno_exit(optarg);
                        break;

                case 'n':
			file_draw = 1;
                        file_name = optarg;
                        break;

                case 's':
                        clear_screen = 1;
                        break;

                default:
                        usage(stderr, argc, argv);
                        exit(EXIT_FAILURE);
                }
        }

        open_device();

	if(file_draw) {
		int fd_file;

#if 1
#if 1
                fmt.fmt.pix.width       = 320;
                fmt.fmt.pix.height      = 240;
                fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
                fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
#else

                if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt))
                        errno_exit("VIDIOC_G_FMT");
#endif
#endif

		file_buf = malloc(MAX_FILE_BUF);
		if(file_buf == NULL) {
                        exit(EXIT_FAILURE);
		}

		file_draw = 0;
		fd_file = open(file_name, O_RDONLY);
		if(fd_file == -1) {
			errno_exit("file open");
		}
		read(fd_file, file_buf, MAX_FILE_BUF);
		capture_to_fb(file_buf, MAX_FILE_BUF, xpos, ypos, disp_ratio);
		close(fd_file);
		free(file_buf);

		return 0;
	}		

        init_device();
        start_capturing();
        mainloop();
        stop_capturing();
        uninit_device();
        close_device();
        return 0;
}
