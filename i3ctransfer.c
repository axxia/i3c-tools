// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 Synopsys, Inc. and/or its affiliates.
 *
 * Author: Vitor Soares <vitor.soares@synopsys.com>
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <i3c/i3cdev.h>

#define VERSION "0.1"

const char *sopts = "d:a:r:w:c:hv";
static const struct option lopts[] = {
	{"device",		required_argument,	NULL,	'd' },
	{"read",		required_argument,	NULL,	'r' },
	{"write",		required_argument,	NULL,	'w' },
	{"combo",               required_argument,      NULL,   'c' },
	{"help",		no_argument,		NULL,	'h' },
	{"version",		no_argument,		NULL,	'v' },
	{0, 0, 0, 0}
};

static void print_usage(const char *name)
{
	fprintf(stderr,
		"  usage: %s options...\n"
 		"   note: -r, -w, and -c are exclusive.  One, and only one, must be specified.\n", name);
	fprintf(stderr,
		"options:\n");
	fprintf(stderr,
		"    -d --device       <dev>\n"
		"        REQUIRED: Device to use.\n");
	fprintf(stderr,
		"    -r --read         [address]:<data length>\n"
		"        Address for I2C transfer, empty for I3C.  Number of bytes to read.\n");
	fprintf(stderr,
		"    -w --write        [address]:<data block>\n"
		"        Address for I2C transfer, emptyr for I3C.  Write data block.\n");
	fprintf(stderr,
		"    -c --combo        [address]:<offset>:r|w:<data length>|<data block>\n"
		"        Address for I2C transfers, empty for I3C\n"
		"        <offset> 16 bit offset for initial write.\n"
		"        Select read or write for secondary operation.\n"
		"        Length if read or write data block if write.\n");
	fprintf(stderr,
		"    -h --help\n"
		"        Output usage message and exit.\n");
	fprintf(stderr,
		"    -v --version\n"
		"        Output the version number and exit\n");
}

static int r_args_to_xfer(struct i3c_ioc_priv_xfer *xfer, char *arg)
{
	char *tmp;
	int len;

	memset(xfer, 0, sizeof(*xfer));
	xfer->rnw = 1;

	/**
	 * Address (optional) -- Implies i2cni3c if it is present.
	 */

	if (strchr(arg, ':') != NULL) {
		if (arg[0] != ':') {
			tmp = strtok(arg, ":");

			if (!tmp) {
				fprintf(stderr,
					"%s failed at %d\n",
					__func__, __LINE__);

				return -1;
			}

			xfer->i2cni3c = 1;
			xfer->addr = strtol(tmp, NULL, 0);

			tmp = strtok(NULL, ":");

			if (!tmp) {
				fprintf(stderr,
					"%s failed at %d\n",
					__func__, __LINE__);

				return -1;
			}
		} else {
			tmp = arg;
			++tmp;
			printf("%d - tmp=%s\n", __LINE__, tmp);
		}
	} else {
		tmp = arg;	
	}

	/**
	 * Allocate a buffer for the read.
	 */

	len = strtol(tmp, NULL, 0);

	tmp = (char *)calloc(len, sizeof(uint8_t));

	if (!tmp) {
		fprintf(stderr, "%s failed at %d\n", __func__, __LINE__);

		return -1;
	}

	xfer->len = len;
	xfer->data = (uintptr_t)tmp;

	return 0;
}

static int w_args_to_xfer(struct i3c_ioc_priv_xfer *xfer, char *arg)
{
	char *data_ptrs[256];
	int len, i = 0;
	char *tmp;

	memset(xfer, 0, sizeof(*xfer));
	xfer->rnw = 0;

	/**
	 * Address (optional) -- Implies i2cni3c if it is present.
	 */

	if (strchr(arg, ':') != NULL) {
		if (arg[0] != ':') {
			tmp = strtok(arg, ":");

			if (!tmp) {
				fprintf(stderr,
					"%s failed at %d\n",
					__func__, __LINE__);

				return -1;
			}

			xfer->i2cni3c = 1;
			xfer->addr = strtol(tmp, NULL, 0);

			tmp = strtok(NULL, ":");

			if (!tmp) {
				fprintf(stderr,
					"%s failed at %d\n",
					__func__, __LINE__);

				return -1;
			}
		} else {
			tmp = arg;
			++tmp;
		}
	} else {
		tmp = arg;	
	}

	/**
	 * Allocate and fill a buffer with data.
	 */

	data_ptrs[i] = strtok(tmp, ",");

	while (data_ptrs[i] && i < 255)
		data_ptrs[++i] = strtok(NULL, ",");

	tmp = (char *)calloc(i, sizeof(uint8_t));

	if (!tmp) {
		fprintf(stderr, "%s failed at %d\n", __func__, __LINE__);

		return -1;
	}

	for (len = 0; len < i; len++)
		tmp[len] = (uint8_t)strtol(data_ptrs[len], NULL, 0);

	xfer->len = len;
	xfer->data = (uintptr_t)tmp;

	return 0;
}

static int c_args_to_xfer(struct i3c_ioc_priv_xfer *xfer, char *arg)
{
	char *data_ptrs[256];
	int i, len;
	char *tmp;

	memset(xfer, 0, sizeof(*xfer));

	xfer->combo = 1;

	/**
	 * Address (optional) -- Implies i2cni3c if it is present.
	 */

	tmp = strtok(arg, ":");

	if (!tmp) {
		fprintf(stderr, "%s failed at %d\n", __func__, __LINE__);

		return -1;
	}

	if (0 != strcmp(tmp, "")) {
		xfer->i2cni3c = 1;
		xfer->addr = strtol(tmp, NULL, 0);
	}

	/**
	 * Offset (required).
	 */

	tmp = strtok(NULL, ":");

	if (!tmp) {
		fprintf(stderr, "%s failed at %d\n", __func__, __LINE__);

		return -1;
	}

	xfer->offset = strtol(tmp, NULL, 0);

	if (xfer->offset > 0xffff) {
		fprintf(stderr, "Offset must be <= 0xffff!\n");

		return -1;
	}

	/**
	 * Read or write (required).
	 */

	tmp = strtok(NULL, ":");

	if (!tmp) {
		fprintf(stderr, "%s failed at %d\n", __func__, __LINE__);

		return -1;
	}

	if (0 == strcmp(tmp, "r")) {
		xfer->rnw = 1;
	} else if (0 == strcmp(tmp, "w")) {
		xfer->rnw = 0;
	} else {
		fprintf(stderr, "%s failed at %d\n", __func__, __LINE__);

		return -1;	/* Must be either read or write... */
	}

	/**
	 * Either allocate a buffer (read) or allocate and fill a
	 * buffer with data (write).
	 */
	tmp = strtok(NULL, ":");

	if (!tmp) {
		fprintf(stderr, "%s failed at %d\n", __func__, __LINE__);

		return -1;
	}

	if (xfer->rnw) {
		len = strtol(tmp, NULL, 0);
		tmp = (char *)calloc(len, sizeof(uint8_t));

		if (!tmp) {
			fprintf(stderr,
				"%s failed at %d\n", __func__, __LINE__);

			return -1;
		}

		xfer->data = (uintptr_t)tmp;
	} else {
		i = 0;
		data_ptrs[i] = strtok(tmp, ",");

		while (data_ptrs[i] && i < 255)
			data_ptrs[++i] = strtok(NULL, ",");

		tmp = (char *)calloc(i, sizeof(uint8_t));

		if (!tmp) {
			fprintf(stderr,
				"%s failed at %d\n", __func__, __LINE__);

			return -1;
		}

		for (len = 0; len < i; len++)
			tmp[len] = (uint8_t)strtol(data_ptrs[len], NULL, 0);

		xfer->data = (uintptr_t)tmp;
	}

	xfer->len = len;

	return 0;
}

static void print_rx_data(struct i3c_ioc_priv_xfer *xfer)
{
	uint8_t *tmp;
	int i;

	tmp = (uint8_t *)calloc(xfer->len, sizeof(uint8_t));
	if (!tmp)
		return;

	memcpy(tmp, (void *)(uintptr_t)xfer->data, xfer->len * sizeof(uint8_t));

	fprintf(stdout, "  received data:\n");
	for (i = 0; i < xfer->len; i++)
		fprintf(stdout, "    0x%02x\n", tmp[i]);

	free(tmp);
}

int main(int argc, char *argv[])
{
	struct i3c_ioc_priv_xfer *xfers;
	int file, ret, opt, i;
	int nxfers = 0;
	char *device;

	while ((opt = getopt_long(argc, argv,  sopts, lopts, NULL)) != EOF) {
		switch (opt) {
		case 'h':
			print_usage(argv[0]);
			exit(EXIT_SUCCESS);
			/* fall through */
		case 'v':
			fprintf(stderr, "%s - %s\n", argv[0], VERSION);
			exit(EXIT_SUCCESS);
			/* fall through */
		case 'd':
			device = optarg;
			break;
		case 'r':
		case 'w':
		case 'c':
			nxfers++;
			break;
		default:
			print_usage(argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	if (!device)
		exit(EXIT_FAILURE);

	file = open(device, O_RDWR);
	if (file < 0)
		exit(EXIT_FAILURE);

	xfers = (struct i3c_ioc_priv_xfer *)calloc(nxfers, sizeof(*xfers));
	if (!xfers)
		exit(EXIT_FAILURE);

	optind = 1;
	nxfers = 0;

	while ((opt = getopt_long(argc, argv, sopts, lopts, NULL)) != EOF) {
		switch (opt) {
		case 'r':
			if (r_args_to_xfer(&xfers[nxfers], optarg)) {
				fprintf(stderr, "r_args_to_xfer() failed!\n");
				ret = EXIT_FAILURE;
				goto err_free;
			}

			nxfers++;
			break;
		case 'w':
			if (w_args_to_xfer(&xfers[nxfers], optarg)) {
				fprintf(stderr, "w_args_to_xfer() failed!\n");
				ret = EXIT_FAILURE;
				goto err_free;
			}

			nxfers++;
			break;
		case 'c':
			if (c_args_to_xfer(&xfers[nxfers], optarg)) {
				fprintf(stderr, "c_args_to_xfer() failed!\n");
				ret = EXIT_FAILURE;
				goto err_free;
			}

			nxfers++;
			break;
		default:
			break;
		}
	}

	if (ioctl(file, I3C_IOC_PRIV_XFER(nxfers), xfers) < 0) {
		fprintf(stderr,
			"Error: transfer failed: %s\n", strerror(errno));
		ret = EXIT_FAILURE;
		goto err_free;
	}

	for (i = 0; i < nxfers; i++) {
		fprintf(stdout, "Success on message %d\n", i);
		if (xfers[i].rnw)
			print_rx_data(&xfers[i]);
	}

	ret = EXIT_SUCCESS;

err_free:
	for (i = 0; i < nxfers; i++)
		free((void *)(uintptr_t)xfers[i].data);
	free(xfers);

	return ret;
}
