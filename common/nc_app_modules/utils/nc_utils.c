#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/sendfile.h>  // sendfile
#include <fcntl.h>         // open
#include <unistd.h>        // close
#include <sys/stat.h>      // fstat
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <stdint.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <linux/rtc.h>
#include <arpa/inet.h>
#include <execinfo.h>
#include <sys/mman.h>

#include "nc_utils.h"

typedef struct mmap_
{
    unsigned long   addr;
    unsigned long   length;

    void            *virt_addr;
    void            *mapping;
    int             devmem;
    off_t           map_base;
    off_t           extra_bytes;
    long            page_size;
} mmap_t;

static mmap_t *mm = NULL;

/*
    Functions of related with File
*/
int nc_is_file(const char *path)
{
    struct stat path_stat;

    memset((void *)&path_stat, 0, sizeof(struct stat));
    stat(path, &path_stat);
    if (S_ISREG(path_stat.st_mode))
        return 1;

    return 0;
}

int nc_copy_file(const char *src, const char *dst)
{
    int res;
    int source = open(src, O_RDONLY, 0);
    int dest = open(dst, O_WRONLY | O_CREAT, 0644);

    // struct required, rationale: function stat() exists also
    struct stat stat_source;
    fstat(source, &stat_source);

    res = (int)sendfile(dest, source, 0, stat_source.st_size);

    close(source);
    close(dest);

    return res;
}

int nc_get_file_size(char *file_path)
{
    int size = 0;
    FILE *f;

    f = fopen(file_path, "r");
    if (f != NULL) {
        fseek(f, 0, SEEK_END);
        size = (int)ftell(f);
        fclose(f);
    }

    return size;
}

uint64_t nc_get_free_mem_size(void)
{
       FILE *fp = NULL;
    char *ptr, buf[128] = {0,};
    uint32_t free_kb = 0, buffers_kb = 0, cached_kb = 0;

    fp = fopen("/proc/meminfo", "r");
    if (!fp) {
        return 0ull;
    }

    /*
     # cat /proc/meminfo
        MemTotal:          55440 kB
        MemFree:            8360 kB
        Buffers:            3004 kB
        Cached:            10964 kB
        SwapCached:            0 kB
    */

    while (fgets(buf, sizeof(buf), fp)) {
        if((ptr = strstr(buf, "MemFree:")) != NULL) {
            ptr += 8;
            while (*ptr != 0 && isspace(*ptr))
                ptr++;
            sscanf(ptr, "%u kB", &free_kb);
        } else if((ptr = strstr(buf, "Buffers:")) != NULL) {
            ptr += 8;
            while (*ptr != 0 && isspace(*ptr))
                ptr++;
            sscanf(ptr, "%u kB", &buffers_kb);
        } else if((ptr = strstr(buf, "Cached:")) != NULL) {
            ptr += 7;
            while (*ptr != 0 && isspace(*ptr))
                ptr++;
            sscanf(ptr, "%u kB", &cached_kb);
            break;
        }
    }

    fclose(fp);

    uint64_t total = (free_kb + buffers_kb + cached_kb) * 1024ll;

    // printf("%u %u %u %llu\n", free_kb, buffers_kb, cached_kb, total);

    return total;
}

/*
    Functions of related with Time
*/
uint64_t nc_get_mono_time(void)
{
    struct timespec t;
    uint64_t utime;

    clock_gettime(CLOCK_MONOTONIC, &t);
    utime = (t.tv_sec * 1000) + (t.tv_nsec / 1000 / 1000);

    return utime;
}

uint64_t nc_elapsed_time(uint64_t p_time)
{
    uint64_t e_time;

    e_time = nc_get_mono_time() - p_time;

    return e_time;
}

uint64_t nc_get_mono_us_time(void)
{
    struct timespec t;
    uint64_t utime;

    clock_gettime(CLOCK_MONOTONIC, &t);
    utime = (t.tv_sec * 1000000) + (t.tv_nsec / 1000);

    return utime;
}

uint64_t nc_elapsed_us_time(uint64_t p_time)
{
    uint64_t e_time;

    e_time = nc_get_mono_us_time() - p_time;

    return e_time;
}

#define MAX_PATH_LENGTH    256
static char PATH_PREFIX[MAX_PATH_LENGTH] = {0,};
static char path_buffer[MAX_PATH_LENGTH] = {0,};
int nc_init_path_localizer(void)
{
    const char *app_path = getenv("APP_PATH");

    if (app_path) {
        strncpy(PATH_PREFIX, app_path, MAX_PATH_LENGTH - 1);
        PATH_PREFIX[MAX_PATH_LENGTH - 1] = '\0';
    } else {
        strncpy(PATH_PREFIX, ".", MAX_PATH_LENGTH - 1);
        PATH_PREFIX[MAX_PATH_LENGTH - 1] = '\0';
    }

    strcat(PATH_PREFIX, "/");

    return 0;
}

char *nc_localize_path(const char *path)
{
    memset(path_buffer, 0, MAX_PATH_LENGTH);
    strncpy(path_buffer, PATH_PREFIX, MAX_PATH_LENGTH);
    strcat(path_buffer, path);

    printf("<%s> %s to %s\n", __func__, path, path_buffer);
    return path_buffer;
}

int nc_get_local_IPv4(const char* if_name, char *ret_ip_addr)
{
    int ret = 0;
    int sock_fd;
    struct ifreq ifr;

    // AF_INET - for ip v4
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

    ifr.ifr_addr.sa_family = AF_INET;
    memcpy(ifr.ifr_name, if_name, strlen(if_name));
    if ((ret = ioctl(sock_fd, SIOCGIFADDR, &ifr)) < 0) {
        return ret;
    }

    close(sock_fd);

    strcpy(ret_ip_addr, inet_ntoa(((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr));

    return ret;
}

void nc_backtrace(void)
{
    void *callstack[512] = {0,};
    int i, nr_frames;
    char **strs;

    nr_frames = backtrace(callstack, sizeof(callstack)/sizeof(void *));
    strs = backtrace_symbols(callstack, nr_frames);
    for (i = 0; i < nr_frames; i++) {
        printf("%s\n", strs[i]);
    }
    free(strs);
}

void nc_fps_delay(long milliseconds)
{
    struct timespec ts;

    if(milliseconds<33)
    {
        long delay_time = 33 - milliseconds;

        if(delay_time>0)
        {
            ts.tv_sec = delay_time / 1000;
            ts.tv_nsec = (long)(delay_time % 1000) * 1000000;
            nanosleep(&ts, NULL);
        }
    }
}

uint64_t nc_get_us_from_timeval(struct timeval *ts)
{
    return ts->tv_sec*1000*1000 + ts->tv_usec;
}

uint32_t nc_trim(char *str)
{
    char *start;
    char *end;

    if(str == NULL) {
        return 0;
    }

    start = str;
    end = str + strlen(str) - 1;

    while (*start && isspace(*start)) {
        start++;
    }

    while (end > start && isspace(*end)) {
        *end = '\0';
        end--;
    }

    if (start > str) {
        memmove(str, start, end - start + 2);
    }

    return 0;
}

int nc_mm_map(unsigned long addr)
{
    if(mm == NULL){
        mm = (mmap_t*)malloc(sizeof(mmap_t));
        if(mm == NULL){
            return -1;
        }
        memset(mm, 0, sizeof(mmap_t));
    }

    if( mm->devmem == 0){
        mm->devmem = open("/dev/mem", O_RDWR | O_SYNC);
        if (mm->devmem == -1) {
            perror("Could not open /dev/mem");
            free(mm);
            return -1;
        }
    }

    mm->addr = addr;
    mm->length = 8;
    mm->page_size = sysconf(_SC_PAGE_SIZE);
    mm->map_base = addr & ~(mm->page_size - 1);
    mm->extra_bytes = addr - mm->map_base;

    mm->mapping = mmap(NULL, mm->length + mm->extra_bytes,  PROT_READ | PROT_WRITE, MAP_SHARED, mm->devmem, mm->map_base);
    if (mm->mapping == MAP_FAILED) {
        perror("Could not map memory");
        close(mm->devmem);
        free(mm);
        return -1;
    }

    mm->virt_addr = (void*)((char *)mm->mapping + mm->extra_bytes);

    return 0;
}

void nc_mm_unmap(void)
{
    if(mm != NULL){
        munmap(mm->mapping, mm->length + mm->extra_bytes);
        // close(mm->devmem);
    }
}

unsigned int nc_read_reg(unsigned int addr)
{
    unsigned int data;

    if( nc_mm_map(addr) < 0){
        return -1;
    }

    data = *((unsigned int*)mm->virt_addr);
    nc_mm_unmap();

    return data;
}

int nc_write_reg(unsigned long addr, unsigned long data)
{
    if( nc_mm_map(addr) < 0){
        return -1;
    }

    *((unsigned long*)mm->virt_addr) = data;
    nc_mm_unmap();

    return 0;
}