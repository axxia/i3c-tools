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
		"    -d --device       <device>\n"
		"        REQUIRED: device: Device entry to use.\n");
	fprintf(stderr,
		"    -r --read         [address]:<length>[:<file>]\n"
		"        address: Slave address for I2C transfers, empty for I3C.\n"
		"         length: Number of bytes to read.\n"
		"           file: File to write bytes to.\n");
	fprintf(stderr,
		"    -w --write        [address]:<data>|<file>\n"
		"        address: Slave address for I2C transfers, empty for I3C.\n"
		"           data: Write data.\n"
		"           file: File containing write data.\n");
	fprintf(stderr,
		"    -c --combo        [address]:<offset>:r:<length>[:file] OR\n"
		"                      [address]:<offset>:w:<data>|<file>\n"
		"        address: Slave address for I2C transfers, empty for I3C.\n"
		"         offset: 16 bit offset for initial write.\n"
		"              r: read length bytes.\n"
		"              w: write bytes from command line or file.\n"
		"         length: for reads, number of bytes to read.\n"
		"           data: for writes, data to write.\n"
		"           file: file to read data from, or write data to.\n");
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
	FILE *input;
	char *tmp;

	/**
	 * [address]:<data>|<file>
	 */

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
	 * Allocate and fill a buffer with data.  The data can be
	 * given on the command line as a comma seperated list of
	 * values (0xnn,0xmm,etc.) OR a binary file.
	 */

	/* If 'tmp' is at a file, use it. */

	input = fopen(tmp, "rb");

	if (input) {
		fseek(input, 0L, SEEK_END);
		xfer->len = ftell(input);
		rewind(input);

		tmp = (char *)calloc(xfer->len, sizeof(uint8_t));

		if (!tmp) {
			fprintf(stderr,
				"%s failed at %d\n", __func__, __LINE__);

			return -1;
		}

		fread(tmp, sizeof(char *), xfer->len, input);
		xfer->data = (uintptr_t)tmp;
		fclose(input);
	} else {
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

		xfer->len = len;
		xfer->data = (uintptr_t)tmp;
	}

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

static int handle_read(char *arg, struct i3c_ioc_priv_xfer *xfer)
{
	char *filename;
	FILE *output;
	unsigned char *tmp;
	int i;

	filename = strtok(arg, ":");
	filename = strtok(NULL, ":");
	filename = strtok(NULL, ":");

	if (filename) {
		output = fopen(filename, "w");

		if (!output) {
			fprintf(stderr,
				"fopen(%s) failed: %s\n",
				filename, strerror(errno));

			return -1;
		}
	}

	tmp = (unsigned char *)calloc(xfer->len, sizeof(uint8_t));
	if (!tmp)
		return -1;

	memcpy(tmp, (void *)(uintptr_t)xfer->data, xfer->len * sizeof(uint8_t));

	if (filename) {
		fwrite(tmp, sizeof(char), xfer->len * sizeof(char), output);
		fclose(output);
	} else {
		fprintf(stdout, "  received data:\n");
		for (i = 0; i < xfer->len; i++)
			fprintf(stdout, "    0x%02x\n", tmp[i]);
	}

	free(tmp);

	return 0;
}

int main(int argc, char *argv[])
{
	struct i3c_ioc_priv_xfer *xfers;
	char **xfers_args;
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

	xfers_args = (char **)calloc(nxfers, sizeof(char *));
	if (!xfers_args)
		exit(EXIT_FAILURE);

	optind = 1;
	nxfers = 0;

	while ((opt = getopt_long(argc, argv, sopts, lopts, NULL)) != EOF) {
		switch (opt) {
		case 'r':
			xfers_args[nxfers] = malloc(strlen(optarg) + 1);
			strcpy(xfers_args[nxfers], optarg);

			if (r_args_to_xfer(&xfers[nxfers], optarg)) {
				fprintf(stderr, "r_args_to_xfer() failed!\n");
				ret = EXIT_FAILURE;
				goto err_free;
			}

			nxfers++;
			break;
		case 'w':
			xfers_args[nxfers] = malloc(strlen(optarg) + 1);
			strcpy(xfers_args[nxfers], optarg);

			if (w_args_to_xfer(&xfers[nxfers], optarg)) {
				fprintf(stderr, "w_args_to_xfer() failed!\n");
				ret = EXIT_FAILURE;
				goto err_free;
			}

			nxfers++;
			break;
		case 'c':
			xfers_args[nxfers] = malloc(strlen(optarg) + 1);
			strcpy(xfers_args[nxfers], optarg);

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
		fprintf(stdout,
			"Success on message %d: %s\n", i, xfers_args[i]);
		if (xfers[i].rnw)
			handle_read(xfers_args[i], &xfers[i]);
	}

	ret = EXIT_SUCCESS;

err_free:
	for (i = 0; i < nxfers; i++) {
		free((void *)(uintptr_t)xfers[i].data);
		free(xfers_args[i]);
	}
	free(xfers);
	free(xfers_args);

	return ret;
}
