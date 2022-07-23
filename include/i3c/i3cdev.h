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

#include <uapi/linux/i3c-dev.h>

#define VERSION "0.1"

#define I3C_TOOLS_SIZE(N)	\
	((((sizeof(struct i3c_tools_ioctl)) * (N)) < (1 << _IOC_SIZEBITS)) \
	 ? ((sizeof(struct i3c_tools_ioctl)) * (N)) : 0)

#define I3C_TOOLS_TYPE(N)	\
	_IOC(_IOC_READ|_IOC_WRITE, I3C_TOOLS_IOC_MAGIC, 30, I3C_TOOLS_SIZE(N))

#endif
