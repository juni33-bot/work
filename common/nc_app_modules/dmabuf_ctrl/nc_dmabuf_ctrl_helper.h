#ifndef __NC_DMABUF_CTRL_HELPER_H__
#define __NC_DMABUF_CTRL_HELPER_H__


int nc_dmabuf_ctrl_open(void);
int nc_dmabuf_ctrl_close(void);
int nc_dmabuf_ctrl_alloc_dma_fd(size_t size);
int nc_dmabuf_ctrl_free_dma_fd(int dma_fd);
void nc_dmabuf_ctrl_sync_transfer_dma_fd(int src, int dst);
void nc_dmabuf_ctrl_async_transfer_dma_fd(int src, int dst);
int nc_dmabuf_ctrl_transfer_check(int dma_fd);
void nc_dmabuf_ctrl_alloc_dma_chan(int chan_num);
void nc_dmabuf_ctrl_write_dma_fd(int dma_fd, uint8_t *buf,size_t buffer_size);
void nc_dmabuf_ctrl_read_dma_fd(int dma_fd, uint8_t **buf, size_t buffer_size);
uint32_t nc_dmabuf_ctrl_get_dma_phys(int dma_fd);
void* nc_dmabuf_ctrl_get_dma_virt(int dma_fd);
int nc_dmabuf_ctrl_map_dma_phys(uint32_t phys_addr, size_t buffer_size);
void nc_dmabuf_ctrl_set_cacheable(int dma_fd, bool is_use);
void nc_dmabuf_ctrl_begin_cpu_access(int dma_fd);
void nc_dmabuf_ctrl_end_cpu_access(int dma_fd);


/* interface when applying dmabuf to userspace process */
#define custom_map(dmabuf_fd, process_function, buffer_size)                                            \
(                                                                                                       \
    {                                                                                                   \                                                                                \
        uint8_t* buf;                                                                                   \
        buf = (uint8_t*)mmap(NULL, buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, dmabuf_fd, 0);      \
        nc_dmabuf_ctrl_begin_cpu_access(dmabuf_fd);                                                     \
        process_function(buf);                                                                          \
        nc_dmabuf_ctrl_end_cpu_access(dmabuf_fd);                                                       \
        munmap(buf,buffer_size);                                                                        \
    }                                                                                                   \
)                                                                                                       

#endif /* __NC_DMABUF_CTRL_HELPER_H__ */
