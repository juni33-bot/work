/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * NextChip Inc. DMA_CTRL driver v1.0
 *
 * Copyright (C) 2024 NextChip Inc.  All rights reserved
 */

#ifndef __NC_DMABUF_CTRL_H__
#define __NC_DMABUF_CTRL_H__

enum dmabuf_transfer_opt
{
    TRANSFER_OPT_NOWAIT,
    TRANSFER_OPT_WAIT
};

struct dmabuf_config
{
    size_t                      buffer_size;
    int                         dmabuf_fd;
};

struct dmabuf_transfer
{
    int                         src;
    int                         dst;
    enum dmabuf_transfer_opt    opt;
};

struct dmabuf_rw
{
    int                         dmabuf_fd;
    void                        *userbuf;
    size_t                      buffer_size;
};

struct dmabuf_addr
{
    int                         dmabuf_fd;
    uint32_t                    phys_addr;
    void                        *virt_addr;
};

struct dmabuf_map_phys
{
    int                         dmabuf_fd;
    uint32_t                    phys_addr;
    size_t                      buffer_size;
};

struct dmabuf_cacheable
{
    int                         dmabuf_fd;
    bool                        is_use;
};

#define MAX_CHAN 8

#define IOCTL_DMA_BUF_TRANSFER_SUBMIT       _IOR('C', 0, struct dmabuf_transfer)
#define IOCTL_DMA_BUF_CREATE_FD             _IOWR('C', 1, struct dmabuf_config)
#define IOCTL_DMA_BUF_TRANSFER_CHECK        _IOWR('C', 2, int)
#define IOCTL_DMA_BUF_ALLOC_CHAN            _IOR('C', 3, int)
#define IOCTL_DMA_BUF_DELETE_FD             _IOR('C', 4, int)
#define IOCTL_DMA_BUF_WRITE_FD              _IOR('C', 5, struct dmabuf_rw)
#define IOCTL_DMA_BUF_READ_FD               _IOWR('C', 6, struct dmabuf_rw)
#define IOCTL_DMA_BUF_GET_ADDR              _IOWR('C', 7, struct dmabuf_addr)
#define IOCTL_DMA_BUF_SET_CACHEABLE         _IOR('C', 8, struct dmabuf_cacheable)
#define IOCTL_DMA_BUF_BEGIN_CPU_ACCESS      _IOR('C', 9, int)
#define IOCTL_DMA_BUF_END_CPU_ACCESS        _IOR('C', 10, int)
#define IOCTL_DMA_BUF_PHYS_MAP_CREATE_FD    _IOWR('C', 11, struct dmabuf_map_phys)
#endif /* __NC_DMABUF_CTRL_H__ */
