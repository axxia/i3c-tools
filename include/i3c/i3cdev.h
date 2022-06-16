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

/* IOCTL Defines */

#define I3C_TOOLS_IOC_MAGIC 0x07

enum i3c_tools_ioctl_type {
	I3C_TOOLS_PRIV_XFER,
	I3C_TOOLS_CCC
};

/**
 * struct i3c_tools_ioctl - I2C/I3C Private Transfer OR CCC Command
 * @type: Indicate the type (private transfer or CCC command).
 * @data: Pointer to userspace buffer with transmit/receive data.
 * @len: Length of the buffer, in bytes.
 * @addr: The address of the slave.
 * @offset: For combo transfers.
 * @combo: combo transfer.
 * @i2cni3c: I2C slave.
 * @rnw: Transfer direction. true for a read, false for a write.
 * @ccc: The CCC command ID.
 * @tocwa: Always end with TOC=1.
 */

struct i3c_tools_ioctl {
	enum i3c_tools_ioctl_type type;
	__u64 data;
	__u16 len;
	__u8  addr;
	__u16 offset;
	__u8  combo;
	__u8  i2cni3c;
	__u8  rnw;
	__u8  ccc;
	__u8  tocwa;
};

#define I3C_TOOLS_SIZE(N)	\
	((((sizeof(struct i3c_tools_ioctl)) * (N)) < (1 << _IOC_SIZEBITS)) \
	 ? ((sizeof(struct i3c_tools_ioctl)) * (N)) : 0)

#define I3C_TOOLS_TYPE(N)	\
	_IOC(_IOC_READ|_IOC_WRITE, I3C_TOOLS_IOC_MAGIC, 30, I3C_TOOLS_SIZE(N))

#endif
