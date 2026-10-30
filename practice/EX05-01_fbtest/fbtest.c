#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#define FBDEV	"/dev/fb0"

#define COLOR_RED 0xff0000
#define COLOR_GREEN 0x00ff00
#define COLOR_BLUE 0x0000ff
#define COLOR_BLACK 0x000000
#define COLOR_WHITE 0xffffff

pid_t pid;

void draw_rect(int x, int y, int w, int h, unsigned int color, struct fb_var_screeninfo *vip, struct fb_fix_screeninfo *fip, char *map)
{
	int xx, yy;
	int location = 0;

	for(yy = y; yy < (y+h); yy++) {
		for(xx = x; xx < (x+w); xx++) {
			location = (xx+vip->xoffset) * (vip->bits_per_pixel/8) +
				(yy+vip->yoffset) * fip->line_length;
			if (vip->bits_per_pixel == 32) { /* 32 bpp */
				*(unsigned int *)(map + location) = color;
			} else  { /* 16 bpp */
				*(unsigned short *)(map + location) = (unsigned short)color;
			}
		}
	}
}

int main(int argc, char **argv)
{
	int fd;
	int size;
	char *map;

	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;

        if(argc != 1) {
                printf("usage: %s\n", argv[0]);
                return EXIT_FAILURE;
        }
        printf("[%d] running %s\n", pid = getpid(), argv[0]);

	/* open */
	fd = open(FBDEV, O_RDWR);
	if(fd == -1) {
		printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
		return EXIT_FAILURE;
	}
        printf("[%d] %s was opened successfully\n", pid, FBDEV);

	/* get fb_var_screeninfo */
	if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
		printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
		return EXIT_FAILURE;
	}
	printf("[%d] %dx%d %dbpp\n", pid, vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

	/* get fb_fix_screeninfo */
	if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) == -1) {
		printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
		return EXIT_FAILURE;
	}
	printf("[%d] line_length is %d\n", pid, finfo.line_length);


	/* mmap */
	size = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
	map = (char *)mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (map == (char *)-1) {
		printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
		return EXIT_FAILURE;
	}
	printf("[%d] %s was mapped to %p\n", pid, FBDEV, map);

	/* clear screen */
	draw_rect(0, 0, vinfo.xres, vinfo.yres, COLOR_BLACK, &vinfo, &finfo, map);

	/* draw rectangle */
	draw_rect(100, 100, 100, 100, COLOR_RED, &vinfo, &finfo, map);

	/* close */
	munmap(map, size);
	close(fd);

	return 0;
}
