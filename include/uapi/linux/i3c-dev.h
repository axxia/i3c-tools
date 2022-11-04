/* SPDX-License-Identifier: GPL-2.0-only WITH Linux-syscall-note */

#ifndef _UAPI_LINUX_I3C_DEV_H_
#define _UAPI_LINUX_I3C_DEV_H_

/************************** DOCUMENTATION **************************/

/**
 * There are two ioctl commands available.  Private transfers and CCC
 * commands.
 */

#define I3C_TOOLS_IOC_MAGIC 0x7

enum i3c_tools_ioctl_type {
	I3C_TOOLS_PRIV_XFER,
	I3C_TOOLS_COMBO_XFER,
	I3C_TOOLS_CCC
};

/**
 * struct i3c_tools_ioctl -
 *     I2C/I3C Private Transfer, Combo Transfer, or CCC Command
 * @type: Indicate the type.
 * @data: Pointer to userspace buffer.
 * @len: Length of the userspace buffer, in bytes.
 * @addr: The address of the endpoint.
 * @offset: For combo transfers.
 * @i2cni3c: Set if endpoint is I2C.
 * @rnw: Transfer direction. set for a read, zero for a write.
 * @ccc: The CCC command ID.
 */

struct i3c_tools_ioctl {
	enum i3c_tools_ioctl_type type;
	__u64 data;
	__u16 len;
	__u8  addr;
	__u16 offset;
	__u8  i2cni3c;
	__u8  rnw;
	__u8  ccc;
};

#endif	/* _UAPI_LINUX_I3C_DEV_H_ */
