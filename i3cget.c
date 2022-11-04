// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 Synopsys, Inc. and/or its affiliates.
 *
 * Author: Vitor Soares <vitor.soares@synopsys.com>
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <i3c/i3cdev.h>
#include <uapi/linux/i3c-dev.h>

#define VERSION "0.1"

const char *sopts = "2d:e:l:o:hv";
static const struct option lopts[] = {
	{"device", 		required_argument,	NULL,	'd' },
	{"endpoint",		required_argument,	NULL,	'e' },
	{"i2c",                 no_argument,            NULL,   '2' },
	{"length",		required_argument,	NULL,	'l' },
	{"offset",              required_argument,      NULL,   'o' },
	{"help",		no_argument,		NULL,	'h' },
	{"version",		no_argument,		NULL,	'v' },
	{0, 0, 0, 0}
};

static void print_usage(const char *name)
{
	fprintf(stderr,
		"  usage: %s options\n", name);
	fprintf(stderr,
		"options:\n");
	fprintf(stderr,
		"    -d --device        <dev entry>\n"
		"        REQUIRED: device: Device entry to use.\n");
	fprintf(stderr,
		"    -e --endpoint      <number>\n"
		"        REQUIRED: Endpoint address.\n");
	fprintf(stderr,
		"    -2 --i2c\n"
		"        Use i2c mode.\n");
	fprintf(stderr,
		"    -l --length        <0...0xffff>\n"
		"        REQUIRED: Length of the user space buffer.\n");
	fprintf(stderr,
		"    -o --offset        <0...0xffff>\n"
		"        REQUIRED: Offset of the endpoint register.\n");
	fprintf(stderr,
		"    -h --help\n"
		"        Output usage message and exit.\n");
	fprintf(stderr,
		"    -v --version\n"
		"        Output the version number and exit\n");
}

int main(int argc, char *argv[])
{
	unsigned long endpoint, length, offset;
	struct i3c_tools_ioctl combo;
	int file, i, opt, rc;
	char *device;
	int i2c = 0;
	void *data;

	while ((opt = getopt_long(argc, argv,  sopts, lopts, NULL)) != EOF) {
		switch (opt) {
		case '2':
			i2c = 1;
			break;
		case 'd':
			device = optarg;
			break;
		case 'e':
			endpoint = strtoul(optarg, NULL, 0);
			break;
		case 'l':
			length = strtoul(optarg, NULL, 0);
			break;
		case 'o':
			offset = strtoul(optarg, NULL, 0);
			break;
		case 'h':
			print_usage(argv[0]);
			return 0;
			/* fall through */
		case 'v':
			fprintf(stderr, "%s - %s\n", argv[0], VERSION);
			return 0;
			/* fall through */
		default:
			print_usage(argv[0]);
			return -1;
		}
	}

	if (!device) {
		fprintf(stderr, "No device specified!\n");
		print_usage(argv[0]);

		return -1;
	}

	if (endpoint > 0x7f) {
		fprintf(stderr, "Endpoint must be 0...0x7f!\n");
		print_usage(argv[0]);

		return -1;
	}

	if (length > 0xffff) {
		fprintf(stderr, "Length must be 0...0xffff!\n");
		print_usage(argv[0]);

		return -1;
	}

	if (offset > 0xffff) {
		fprintf(stderr, "Offset must be 0...0xffff!\n");
		print_usage(argv[0]);

		return -1;
	}

	file = open(device, O_RDWR);

	if (file < 0) {
		fprintf(stderr,
			"Error: open() failed: %s\n", strerror(errno));

		return -1;
	}

	data = malloc(length);

	if (!data) {
		fprintf(stderr,
			"Error: malloc() failed: %s\n", strerror(errno));
		close(file);

		return -1;
	}

	memset(data, 0, length);

	memset(&combo, 0, sizeof(struct i3c_tools_ioctl));
	combo.type = I3C_TOOLS_COMBO_XFER;
	combo.data = (__u64)data;
	combo.len = length;
	combo.addr = endpoint;
	combo.offset = offset;
	combo.rnw = 1;

	if (i2c)
		combo.i2cni3c = 1;

	rc = ioctl(file, I3C_TOOLS_TYPE(1), &combo);

	if (rc < 0) {
		fprintf(stderr,
			"Error: transfer failed: %s\n", strerror(errno));
		free(data);
		close(file);

		return -1;
	}

	printf("received data: ");

	for (i = 0; i < combo.len; ++i)
		printf("0x%02x ", ((unsigned char *)data)[i]);

	printf("\n");

	free(data);
	close(file);

	return 0;
}
