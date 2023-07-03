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
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <i3c/i3cdev.h>
#include <i3c/i3clib.h>
#include <uapi/linux/i3c-dev.h>

#define VERSION "0.1"

const char *sopts = "c:d:ghr:vw:";
static const struct option lopts[] = {
	{"command",             required_argument,      NULL,   'c' },
	{"device",		required_argument,	NULL,	'd' },
	{"group",		no_argument,		NULL,	'g' },
	{"help",		no_argument,		NULL,	'h' },
	{"read",		required_argument,	NULL,	'r' },
	{"version",		no_argument,		NULL,	'v' },
	{"write",		required_argument,	NULL,	'w' },
	{0, 0, 0, 0}
};

static void print_usage(const char *name)
{
	fprintf(stderr,
		"  usage: %s options...\n"
 		"   note: -r, -w, and -c are exclusive.  One, and only one, must be specified.\n",
		name);
	fprintf(stderr,
		"options:\n");
	fprintf(stderr,
		"    -c --ccc [read]   <id>:r:<address>:<length>[:<file>]\n"
		"    -c --ccc [write]  <id>:w:<address>:<data>|<file>\n"
		"           type: CCC code.\n"
		"        address: Slave address.\n"
		"           data: Write data.\n"
		"           file: File containing data to write.\n");
	fprintf(stderr,
		"    -d --device       <device>\n"
		"        REQUIRED: device: Device entry to use.\n");
	fprintf(stderr,
		"    -g --group\n"
		"        \"Group\" all transactions -- send stop on last.\n"
		"        All transactions must be of the same type\n"
		"        (i2c or i3c), and address.\n");
	fprintf(stderr,
		"    -h --help\n"
		"        Output usage message and exit.\n");
	fprintf(stderr,
		"    -r --read         <type>:<address>:<length>[:<file>]\n"
		"           type: i2c or i3c.\n"
		"        address: Slave address.\n"
		"         length: Number of bytes to read.\n"
		"           file: File to write bytes to.\n");
	fprintf(stderr,
		"    -v --version\n"
		"        Output the version number and exit\n");
	fprintf(stderr,
		"    -w --write        <type>:<address>:<data>|<file>\n"
		"           type: i2c or i3c.\n"
		"        address: Slave address.\n"
		"           data: Write data.\n"
		"           file: File containing data to write.\n");
}

static int c_args_to_xfer(struct i3c_tools_ioctl *xfer, char *arg)
{
	char *data_ptrs[256];
	int len, i = 0;
	FILE *input;
	char *tmp;

	/**
	 * <command>:r:<address>:<length>[:<file>]
	 * <command>:w:<address>:<data>|<file>
	 */

	memset(xfer, 0, sizeof(*xfer));
	xfer->type = I3C_TOOLS_CCC;

	/* Command */

	tmp = strtok(arg, ":");

	if (!tmp) {
		fprintf(stderr, "%s failed at %d\n", __func__, __LINE__);
		return -1;
	}

	xfer->ccc = strtol(tmp, NULL, 0);

	if (xfer->ccc < 0 || xfer->ccc > 0xff) {
		fprintf(stderr, "<command> must be 0x00...0xff\n");
		return -1;
	}

	/* Read or Write */

	tmp = strtok(NULL, ":");

	if (!tmp) {
		fprintf(stderr, "%s failed at %d\n", __func__, __LINE__);
		return -1;
	}

	if (strncmp("r", tmp, strlen("r")) == 0) {
		xfer->rnw = 1;
	} else if (strncmp("w", tmp, strlen("w")) == 0) {
		xfer->rnw = 0;
	} else {
		fprintf(stderr,
			"The second field of a CCC command must be either r (read) or w (write).\n");
		return -1;
	}

	/* Address */
	tmp = strtok(NULL, ":");

	if (!tmp) {
		fprintf(stderr, "%s failed at %d\n", __func__, __LINE__);
		return -1;
	}

	xfer->addr = strtol(tmp, NULL, 0);

	if (xfer->rnw) {
		/* Length */
		tmp = strtok(NULL, ":");

		if (!tmp) {
			fprintf(stderr, "%s failed at %d\n", __func__, __LINE__);
			return -1;
		}

		xfer->len = strtol(tmp, NULL, 0);

		/* Allocate a buffer for the read. */
		tmp = (char *)calloc(xfer->len, sizeof(uint8_t));

		if (!tmp) {
			fprintf(stderr,
				"%s failed at %d\n", __func__, __LINE__);

			return -1;
		}

		xfer->data = (uintptr_t)tmp;
	} else {
		/**
		 * Allocate and fill a buffer with data.  The data can be
		 * given on the command line as a comma seperated list of
		 * values (0xnn,0xmm,etc.) OR a binary file.
		 */

		tmp = strtok(NULL, ":");
		input = fopen(tmp, "rb");

		if (input) {
			fseek(input, 0L, SEEK_END);
			xfer->len = ftell(input);
			rewind(input);

			tmp = (char *)calloc(xfer->len, sizeof(uint8_t));

			if (!tmp) {
				fprintf(stderr,
					"%s failed at %d\n",
					__func__, __LINE__);

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
					"%s failed at %d\n",
					__func__, __LINE__);

				return -1;
			}

			for (len = 0; len < i; len++)
				tmp[len] = (uint8_t)strtol(data_ptrs[len],
							   NULL, 0);

			xfer->len = len;
			xfer->data = (uintptr_t)tmp;
		}
	}

	return 0;
}

static int r_args_to_xfer(struct i3c_tools_ioctl *xfer, char *arg)
{
	char *tmp;
	int len;

	memset(xfer, 0, sizeof(*xfer));
	xfer->rnw = 1;
	xfer->type = I3C_TOOLS_PRIV_XFER;

	/* Type */
	tmp = strtok(arg, ":");

	if (!tmp) {
		fprintf(stderr, "%s failed at %d\n", __func__, __LINE__);
		return -1;
	}

	if (strncmp("i2c", tmp, strlen("i2c")) == 0) {
		xfer->i2cni3c = 1;
	} else if (strncmp("i3c", tmp, strlen("i3c")) == 0) {
		xfer->i2cni3c = 0;
	} else {
		fprintf(stderr, "<type> must be i2c or i3c\n");
		return -1;
	}

	/* Address */
	tmp = strtok(NULL, ":");

	if (!tmp) {
		fprintf(stderr, "%s failed at %d\n", __func__, __LINE__);
		return -1;
	}

	xfer->addr = strtol(tmp, NULL, 0);

	tmp = strtok(NULL, ":");

	if (!tmp) {
		fprintf(stderr, "%s failed at %d\n", __func__, __LINE__);
		return -1;
	}

	len = strtol(tmp, NULL, 0);

	if (len > USHRT_MAX) {
		fprintf(stderr, "%s failed at %d\n", __func__, __LINE__);
		return -1;
	}

	xfer->len = len;

	/* Allocate a buffer for the read. */
	tmp = (char *)calloc(xfer->len, sizeof(uint8_t));

	if (!tmp) {
		fprintf(stderr, "%s failed at %d\n", __func__, __LINE__);

		return -1;
	}

	xfer->data = (uintptr_t)tmp;

	return 0;
}

static int w_args_to_xfer(struct i3c_tools_ioctl *xfer, char *arg)
{
	char *data_ptrs[256];
	int len, i = 0;
	FILE *input;
	char *tmp;

	/**
	 * <type>:<address>:<data>|<file>
	 */

	memset(xfer, 0, sizeof(*xfer));
	xfer->rnw = 0;
	xfer->type = I3C_TOOLS_PRIV_XFER;

	/* Type */
	tmp = strtok(arg, ":");

	if (!tmp) {
		fprintf(stderr, "%s failed at %d\n", __func__, __LINE__);
		return -1;
	}

	if (strncmp("i2c", tmp, strlen("i2c")) == 0) {
		xfer->i2cni3c = 1;
	} else if (strncmp("i3c", tmp, strlen("i3c")) == 0) {
		xfer->i2cni3c = 0;
	} else {
		fprintf(stderr, "<type> must be i2c or i3c\n");
		return -1;
	}

	/* Address */
	tmp = strtok(NULL, ":");

	if (!tmp) {
		fprintf(stderr, "%s failed at %d\n", __func__, __LINE__);
		return -1;
	}

	xfer->addr = strtol(tmp, NULL, 0);

	/**
	 * Allocate and fill a buffer with data.  The data can be
	 * given on the command line as a comma seperated list of
	 * values (0xnn,0xmm,etc.) OR a binary file.
	 */

	tmp = strtok(NULL, ":");
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

static int handle_read(char *arg, struct i3c_tools_ioctl *xfer)
{
	char *filename;
	FILE *output;
	unsigned char *tmp;

	filename = strtok(arg, ":");
	filename = strtok(NULL, ":");
	filename = strtok(NULL, ":");
	filename = strtok(NULL, ":");

	if (xfer->ccc)
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

	tmp = (unsigned char *)xfer->data;

	if (filename) {
		fwrite(tmp, sizeof(char), xfer->len * sizeof(char), output);
		fclose(output);
	} else {
		display("received data", tmp, xfer->len);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	struct i3c_tools_ioctl *xfers;
	int file, ret, opt, i;
	char **xfers_args;
	int nxfers = 0;
	int group = 0;
	char *device;

	while ((opt = getopt_long(argc, argv,  sopts, lopts, NULL)) != EOF) {
		switch (opt) {
		case 'g':
			group = 1;
			break;
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

	xfers = (struct i3c_tools_ioctl *)calloc(nxfers, sizeof(*xfers));
	if (!xfers)
		exit(EXIT_FAILURE);

	xfers_args = (char **)calloc(nxfers, sizeof(char *));
	if (!xfers_args)
		exit(EXIT_FAILURE);

	optind = 1;
	nxfers = 0;

	while ((opt = getopt_long(argc, argv, sopts, lopts, NULL)) != EOF) {
		switch (opt) {
		case 'c':
			if (group) {
				fprintf(stderr,
					"Only r: and w: are allowed with -g\n");
				ret = EXIT_FAILURE;
				goto err_free;
			}

			xfers_args[nxfers] = malloc(strlen(optarg) + 1);
			strcpy(xfers_args[nxfers], optarg);

			if (c_args_to_xfer(&xfers[nxfers], optarg)) {
				fprintf(stderr, "c_args_to_xfer() failed!\n");
				ret = EXIT_FAILURE;
				goto err_free;
			}

			nxfers++;
			break;
		case 'r':
			xfers_args[nxfers] = malloc(strlen(optarg) + 1);
			strcpy(xfers_args[nxfers], optarg);

			if (r_args_to_xfer(&xfers[nxfers], optarg)) {
				fprintf(stderr, "r_args_to_xfer() failed!\n");
				ret = EXIT_FAILURE;
				goto err_free;
			}

			if (group)
				xfers[nxfers].type = I3C_TOOLS_GROUPED_PRIV_XFER;

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

			if (group)
				xfers[nxfers].type = I3C_TOOLS_GROUPED_PRIV_XFER;

			nxfers++;
			break;
		default:
			break;
		}
	}

	printf("nxfers=%d group=%d cmd=0x%lx size=0x%lx sizeof=0x%lx max=0x%x\n",
	       nxfers, group, I3C_TOOLS_TYPE(nxfers), I3C_TOOLS_SIZE(nxfers),
	       sizeof(struct i3c_tools_ioctl), (1 << _IOC_SIZEBITS));

	if (ioctl(file, I3C_TOOLS_TYPE(nxfers), xfers) < 0) {
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
