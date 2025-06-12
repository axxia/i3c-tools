/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * Copyright (c) 2020 Synopsys, Inc. and/or its affiliates.
 *
 * Author: Vitor Soares <vitor.soares@synopsys.com>
 */

#ifndef _UAPI_I3C_DEV_H_
#define _UAPI_I3C_DEV_H_

#include <linux/types.h>
#include <linux/ioctl.h>

/* IOCTL commands */
#define I3C_DEV_IOC_MAGIC	0x6d

/**
 * struct i3c_ioc_priv_xfer - I3C SDR ioctl private transfer
 * @data: Holds pointer to userspace buffer with transmit data.
 * @len: Length of data buffer buffers, in bytes.
 * @rnw: encodes the transfer direction. true for a read, false for a write
 */
struct i3c_ioc_priv_xfer {
	__u64 data;
	__u16 len;
	__u8 rnw;
	__u8 pad[5];
};

/**
 * struct i3c_ioc_priv_xfers_args - I3C SDR ioctl private transfer request
 * @nxfers: number of transfers in batch
 * @xfers: Holds pointer to userspace array of struct i3c_ioc_priv_xfer
 */
struct i3c_ioc_priv_xfers_args {
	__u64 nxfers;
	__u64 xfers;
};


/* struct i3c_ioc_ibi_reg_args - I3C in-band interrupt register
 * @num_slots: Maximum number of IBIs to hold in HW retrieval pool
 * @ring_size: Maximum number of IBIs to hold in driver SW ring
 * @max_payload_length: Maximum length in bytes of IBI payload
 */
struct i3c_ioc_ibi_reg_args {
	__u16 num_slots;
	__u16 ring_size;
	__u16 max_payload_length;
};

/**
 * struct i3c_ioc_ibi_wait_args - I3C in-band interrupt wait
 * @wait_time_us: Number of us to wait for an IBI to arrive. A value of -1
 *                indicates to wait indefinitely.
 * @payload: Holds pointer to userspace buffer where IBI payload is stored
 */
struct i3c_ioc_ibi_wait_args {
	__u32 wait_time_us;
	__u16 payload_len;
	__u8 pad[2];
	__u64 payload;
};

#define I3C_IOC_PRIV_XFERS _IOWR(I3C_DEV_IOC_MAGIC, 1, struct i3c_ioc_priv_xfers_args)

#define I3C_IOC_IBI_REG _IOW(I3C_DEV_IOC_MAGIC, 2, struct i3c_ioc_ibi_reg_args)

#define I3C_IOC_IBI_UNREG _IO(I3C_DEV_IOC_MAGIC, 3)

#define I3C_IOC_IBI_WAIT _IOWR(I3C_DEV_IOC_MAGIC, 4, struct i3c_ioc_ibi_wait_args)

#endif
