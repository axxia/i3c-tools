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
#include <asm/ioctl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <linux/i3c/i3cdev.h>

#include "i3clib.h"

#define VERSION "0.1"

const char *sopts = "d:ht:v";
static const struct option lopts[] = {
	{"device",		required_argument,	NULL,	'd' },
	{"help",		no_argument,		NULL,	'h' },
	{"timeout",		required_argument,	NULL,	't' },
	{"version",		no_argument,		NULL,	'v' },
	{0, 0, 0, 0}
};

static void print_usage(const char *name)
{
	fprintf(stderr,
		"usage: %s options...\n", name);
	fprintf(stderr,
		"  options:\n");
	fprintf(stderr,
		"    -d|--device <device>    Device to use.  [no default]\n");
	fprintf(stderr,
		"    -h|--help               Output usage message and exit.\n");
	fprintf(stderr,
		"    -t|--timeout <ms>       Number of ms to wait.  -1 means wait forever.  [default is -1]\n");
	fprintf(stderr,
		"    -v|--version            Output the version number and exit.\n");
}

int main(int argc, char *argv[])
{
	int file, ret, opt;
	struct i3c_ioc_ibi_reg_args reg_args;
	struct i3c_ioc_ibi_wait_args wait_args;
	unsigned char wait_buf[256];
	char *device;
	int timeout = -1;

	while ((opt = getopt_long(argc, argv,  sopts, lopts, NULL)) != EOF) {
		switch (opt) {
		case 'd':
			device = optarg;
			break;
		case 'h':
			print_usage(argv[0]);
			exit(EXIT_SUCCESS);
			/* fall through */
		case 't':
			timeout = strtol(optarg, (char **)NULL, 0);
			break;
		case 'v':
			fprintf(stderr, "%s - %s\n", argv[0], VERSION);
			exit(EXIT_SUCCESS);
			/* fall through */
		default:
			print_usage(argv[0]);
			exit(EXIT_FAILURE);
			/* fall through */
		}
	}

	if (!device)
		exit(EXIT_FAILURE);

	file = open(device, O_RDWR);
	if (file < 0)
		exit(EXIT_FAILURE);

	reg_args.num_slots = 32;
	reg_args.ring_size = 256;
	reg_args.max_payload_length = 32;

	ret = ioctl(file, I3C_IOC_IBI_REG, &reg_args);
	if (ret) {
		fprintf(stderr, "Error: IBI registration failed: %s\n",
			strerror(errno));
		return ret;
	}

	wait_args.wait_time_us = timeout * 1000;
	wait_args.payload_len = 16;
	wait_args.payload = (__u64)wait_buf;

	ret = ioctl(file, I3C_IOC_IBI_WAIT, &wait_args);
	if (ret) {
		fprintf(stderr, "Error: IBI wait failed: %s\n", strerror(errno));
		return ret;
	}

	printf("Got an IBI!\n"
	       "    wait_time_us=%d\n"
	       "    payload_len=%d\n",
	       wait_args.wait_time_us, wait_args.payload_len);

	if (wait_args.payload_len)
		display("received data",
			(void *)wait_args.payload, wait_args.payload_len);

	ret = ioctl(file, I3C_IOC_IBI_UNREG, NULL);
	if (ret) {
		fprintf(stderr, "Error: IBI unregistration failed: %s\n",
			strerror(errno));
		return ret;
	}

	return 0;
}
