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
#include <i3c/i3clib.h>
#include <uapi/linux/i3c-dev.h>

#define VERSION "0.1"

const char *sopts = "d:hv";
static const struct option lopts[] = {
	{"device", 		required_argument,	NULL,	'd' },
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
		"    -h --help\n"
		"        Output usage message and exit.\n");
	fprintf(stderr,
		"    -v --version\n"
		"        Output the version number and exit\n");
}

int main(int argc, char *argv[])
{
	struct i3c_tools_ioctl reset;
	int file, opt, rc;
	char *device;

	while ((opt = getopt_long(argc, argv,  sopts, lopts, NULL)) != EOF) {
		switch (opt) {
		case 'd':
			device = optarg;
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

	file = open(device, O_RDWR);

	if (file < 0) {
		fprintf(stderr,
			"Error: open() failed: %s\n", strerror(errno));

		return -1;
	}

	memset(&reset, 0, sizeof(struct i3c_tools_ioctl));
	reset.type = I3C_TOOLS_RESET;

	rc = ioctl(file, I3C_TOOLS_TYPE(1), &reset);

	if (rc < 0) {
		fprintf(stderr,
			"Error: transfer failed: %s\n", strerror(errno));
		close(file);
		return -1;
	}

	close(file);
	return 0;
}
