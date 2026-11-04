#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/lp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <termios.h>
#include <errno.h>
#include <sys/signal.h>
#include <time.h>
#include <signal.h>
#include <poll.h>
#include <mqueue.h>
#include <sys/eventfd.h>
#include <sys/time.h>

#include "nc_dmabuf_ctrl_helper.h"
#include "nc_dmabuf_ctrl.h"
#include "nc_utils.h"

/*
 ********************************************************************************
 *               DEFINES
 ********************************************************************************
 */

#define DEV_FILE_DMABUF_CTRL   "/dev/nc_dmabuf_ctrl"

/*
 ********************************************************************************
 *               VARIABLE DECLARATIONS
 ********************************************************************************
 */
static int g_dmabuf_ctrl_dev = -1;


/*
 ********************************************************************************
 *               TYPEDEFS
 ********************************************************************************
 */

/*
 ********************************************************************************
 *               LOCAL FUNCTION DECLARATIONS
 ********************************************************************************
 */


/*
 ********************************************************************************
 *               LOCAL FUNCTION DECLARATIONS
 ********************************************************************************
 */



/*
 ********************************************************************************
 *               INTERFACE FUNCTION DEFINITIONS
 ********************************************************************************
 */
int nc_dmabuf_ctrl_open(void)
{
    int ret = 0;

    if(g_dmabuf_ctrl_dev > 0) 
    {
        printf("[dma_ctrl] already dmabuf_ctrl(%s) opened...!\n", DEV_FILE_DMABUF_CTRL);
        ret = -1;
        return ret;
    }

    g_dmabuf_ctrl_dev = open(DEV_FILE_DMABUF_CTRL, O_RDWR | O_SYNC);
    if(g_dmabuf_ctrl_dev < 0) 
    {
        printf("[dmabuf_ctrl] open failure! (%s)\n", DEV_FILE_DMABUF_CTRL);
        ret = -1;
        return ret;
    }

    return ret;
}

int nc_dmabuf_ctrl_close(void)
{
    int ret = 0;

    if(g_dmabuf_ctrl_dev < 0)
    {
        printf("[dma_ctrl] already dma_ctrl(%s) is not open...!\n", DEV_FILE_DMABUF_CTRL);
        ret = -1;
        return ret;
    }
    close(g_dmabuf_ctrl_dev);

    return ret;
}

int nc_dmabuf_ctrl_alloc_dma_fd(size_t size)
{
    int ret = 0;
    dmabuf_config config_value = {0,0};

    config_value.buffer_size = size;
    if(ioctl(g_dmabuf_ctrl_dev,IOCTL_DMA_BUF_CREATE_FD,&config_value) < 0)
    {
        perror("IOCTL_CREATE_DMA_BUF_FD");
        ret = -1;
        return ret;
    }
    ret = config_value.dmabuf_fd;
    
    return ret;
}

int nc_dmabuf_ctrl_free_dma_fd(int dma_fd)
{
    int ret = 0;

    if(ioctl(g_dmabuf_ctrl_dev,IOCTL_DMA_BUF_DELETE_FD,&dma_fd) < 0)
    {
        perror("IOCTL_DMA_BUF_DELETE_FD");
        ret = -1;
        return ret;
    }
    return ret;
}

void nc_dmabuf_ctrl_sync_transfer_dma_fd(int src, int dst)
{
    struct dmabuf_transfer transfer;
    transfer.src = src;
    transfer.dst = dst;
    transfer.opt = TRANSFER_OPT_WAIT;
    
    if(ioctl(g_dmabuf_ctrl_dev,IOCTL_DMA_BUF_TRANSFER_SUBMIT,&transfer)<0)
    {
        perror("IOCTL_TRANSFER_DMA_BUF");
    }
}

void nc_dmabuf_ctrl_async_transfer_dma_fd(int src, int dst)
{
    struct dmabuf_transfer transfer;
    transfer.src = src;
    transfer.dst = dst;
    transfer.opt = TRANSFER_OPT_NOWAIT;
    
    if(ioctl(g_dmabuf_ctrl_dev,IOCTL_DMA_BUF_TRANSFER_SUBMIT,&transfer)<0)
    {
        perror("IOCTL_TRANSFER_DMA_BUF");
    }
}

int nc_dmabuf_ctrl_transfer_check(int dma_fd)
{
    int ret = 0;
    int dmabuf_fd = dma_fd;
    
    if(ioctl(g_dmabuf_ctrl_dev,IOCTL_DMA_BUF_TRANSFER_CHECK,&dmabuf_fd)<0)
    {
        perror("IOCTL_TRANSFER_CHECK");
    }

    if(dma_fd < 0)
    {
        ret = -1;
    }

    return ret;
}

void nc_dmabuf_ctrl_alloc_dma_chan(int chan_num)
{
    if(ioctl(g_dmabuf_ctrl_dev,IOCTL_DMA_BUF_ALLOC_CHAN,&chan_num) < 0)
    {
        perror("IOCTL_DMA_BUF_ALLOC_CHAN");
    }

}

void nc_dmabuf_ctrl_write_dma_fd(int dma_fd, uint8_t *buf, size_t buffer_size)
{
    struct dmabuf_rw write;

    write.dmabuf_fd = dma_fd;
    write.userbuf = buf;
    write.buffer_size = buffer_size;

    if(ioctl(g_dmabuf_ctrl_dev,IOCTL_DMA_BUF_WRITE_FD,&write) < 0)
    {
        perror("IOCTL_DMA_BUF_WRITE_FD");
    }
}

void nc_dmabuf_ctrl_read_dma_fd(int dma_fd, uint8_t **buf, size_t buffer_size)
{
    struct dmabuf_rw read;
    
    read.dmabuf_fd = dma_fd;
    read.userbuf = buf;
    read.buffer_size = buffer_size;

    if(ioctl(g_dmabuf_ctrl_dev,IOCTL_DMA_BUF_READ_FD,&read) < 0)
    {
        perror("IOCTL_DMA_BUF_READ_FD");
    }
}

uint32_t nc_dmabuf_ctrl_get_dma_phys(int dma_fd)
{
    struct dmabuf_addr get_addr;

    get_addr.dmabuf_fd = dma_fd;
    get_addr.phys_addr = 0;
    get_addr.virt_addr = NULL;

    if(ioctl(g_dmabuf_ctrl_dev,IOCTL_DMA_BUF_GET_ADDR,&get_addr) < 0)
    {
        perror("IOCTL_DMA_BUF_GET_PHYS");
    }

    return get_addr.phys_addr;
}

void* nc_dmabuf_ctrl_get_dma_virt(int dma_fd)
{
    struct dmabuf_addr get_addr;

    get_addr.dmabuf_fd = dma_fd;
    get_addr.phys_addr = 0;
    get_addr.virt_addr = NULL;

    if(ioctl(g_dmabuf_ctrl_dev,IOCTL_DMA_BUF_GET_ADDR,&get_addr) < 0)
    {
        perror("IOCTL_DMA_BUF_GET_VIRT");
    }
    
    return get_addr.virt_addr;
}

int nc_dmabuf_ctrl_map_dma_phys(uint32_t phys_addr, size_t buffer_size)
{
    struct dmabuf_map_phys map_phys = {0,0,0};

    map_phys.dmabuf_fd = 0;
    map_phys.phys_addr = phys_addr;
    map_phys.buffer_size = buffer_size;

    if(ioctl(g_dmabuf_ctrl_dev,IOCTL_DMA_BUF_PHYS_MAP_CREATE_FD,&map_phys) < 0)
    {
        perror("IOCTL_DMA_BUF_PHYS_MAP_CREATE_FD");
    }

    return map_phys.dmabuf_fd;
}

void nc_dmabuf_ctrl_set_cacheable(int dma_fd, bool is_use)
{
    struct dmabuf_cacheable cacheable;

    cacheable.dmabuf_fd = dma_fd;
    cacheable.is_use = is_use;

    if(ioctl(g_dmabuf_ctrl_dev,IOCTL_DMA_BUF_SET_CACHEABLE,&cacheable) < 0)
    {
        perror("IOCTL_DMABUF_SET_CACHEABLE");
    }
    
}

void nc_dmabuf_ctrl_begin_cpu_access(int dma_fd)
{
    if(ioctl(g_dmabuf_ctrl_dev,IOCTL_DMA_BUF_BEGIN_CPU_ACCESS,&dma_fd) < 0)
    {
        perror("IOCTL_DMA_BUF_BEGIN_CPU_ACCESS");
    }
}

void nc_dmabuf_ctrl_end_cpu_access(int dma_fd)
{
    if(ioctl(g_dmabuf_ctrl_dev,IOCTL_DMA_BUF_END_CPU_ACCESS,&dma_fd) < 0)
    {
        perror("IOCTL_DMA_BUF_END_CPU_ACCESS");
    }
}
