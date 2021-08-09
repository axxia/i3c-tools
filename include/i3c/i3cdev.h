/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2019 Synopsys, Inc. and/or its affiliates.
 *
 * Author: Vitor Soares <vitor.soares@synopsys.com>
 */

#ifndef _UAPI_I3C_DEV_H_
#define _UAPI_I3C_DEV_H_

#include <linux/types.h>
#include <linux/ioctl.h>

#define VERSION "0.1"

/* IOCTL commands */
#define I3C_DEV_IOC_MAGIC	0x07

/**
 * struct i3c_ioc_priv_xfer - I3C SDR ioctl private transfer
 * @data: Holds pointer to userspace buffer with transmit data.
 * @len: Length of data buffer buffers, in bytes.
 * @addr: The address of the I2C slave.
 * @offset: For combo transfers.
 * @combo: combo transfer.
 * @i2cni3c: I2C slave.
 * @rnw: encodes the transfer direction. true for a read, false for a write
 */

struct i3c_ioc_priv_xfer {
	__u64 data;
	__u16 len;
	__u8  addr;	     /* slave address for i2c transfers */
	__u16 offset;	     /* for combo transfers only */
	__u8  combo;	     /* combo, instead of regular, transfer */
	__u8  i2cni3c;
	__u8  rnw;
};

#define I3C_PRIV_XFER_SIZE(N)	\
	((((sizeof(struct i3c_ioc_priv_xfer)) * (N)) < (1 << _IOC_SIZEBITS)) \
	? ((sizeof(struct i3c_ioc_priv_xfer)) * (N)) : 0)

#define I3C_IOC_PRIV_XFER(N)	\
	_IOC(_IOC_READ|_IOC_WRITE, I3C_DEV_IOC_MAGIC, 30, I3C_PRIV_XFER_SIZE(N))

#endif
