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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <i3c/i3cdev.h>
#include <i3c/i3clib.h>
#include <uapi/linux/i3c-dev.h>

#define VERSION "0.1"

const char *sopts = "2b:d:e:hv";
static const struct option lopts[] = {
	{"blocks", 		required_argument,	NULL,	'b' },
	{"device", 		required_argument,	NULL,	'd' },
	{"endpoint",		required_argument,	NULL,	'e' },
	{"i2c",                 no_argument,            NULL,   '2' },
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
		"    -b --blocks <comma separated list of blocks>\n"
		"        A block starts with r: (read) or w: (write)\n"
		"        For reads, r:<length>[:file]\n"
		"        For writes, w:<file>|<comma separated values>\n\n");
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
		"    -h --help\n"
		"        Output usage message and exit.\n");
	fprintf(stderr,
		"    -v --version\n"
		"        Output the version number and exit\n");
}

static int send_ioctl(int file, struct i3c_tools_ioctl *command)
{
	int rc;

	rc = ioctl(file, I3C_TOOLS_TYPE(1), command);

	if (rc < 0) {
		fprintf(stderr,
			"Error: transfer failed: %s\n", strerror(errno));

		return -1;
	}

	return 0;
}

static int block_read(int file, char *block)
{
	char *tmp;
	ssize_t length;
	FILE *output;
	void *buffer;

	tmp = strtok(block, ":");
	tmp = strtok(NULL, ":");
	length = strtoul(tmp, NULL, 0);
	buffer = malloc(length);
	tmp = strtok(NULL, ":");
	output = fopen(tmp, "wb");
	read(file, buffer, length);

	if (output) {
		fwrite(buffer, sizeof(unsigned char), length, output);
		fclose(output);
	} else {
		display("read block", buffer, length);
	}

	free(buffer);

	return 0;
}

static int block_write(int file, char *block)
{
	char *tmp;
	FILE *input;
	ssize_t length;
	void *buffer;

	tmp = strtok(block, ":");
	tmp = strtok(NULL, ":");
	input = fopen(tmp, "rb");

	if (input) {
		fseek(input, 0L, SEEK_END);
		length = ftell(input);
		rewind(input);
		buffer = malloc(length);
		fread(buffer, sizeof(unsigned char), length, input);
	} else {
		char *data_ptrs[256];
		int i = 0;

		data_ptrs[i] = strtok(tmp, ",");

		while (data_ptrs[i] && i < 255)
			data_ptrs[++i] = strtok(NULL, ",");

		length = i;
		buffer = malloc(length);

		for (i = 0; i < length; ++i)
			((unsigned char *)buffer)[i] =
				(unsigned char)strtol(data_ptrs[i], NULL, 0);
	}

	write(file, buffer, length);
	free(buffer);

	return 0;
}

int main(int argc, char *argv[])
{
	struct i3c_tools_ioctl command;
	unsigned long endpoint;
	int file, opt, rc;
	char *blockstr, *blocks[256], *device;
	int i = 0, i2c = 0, nblocks;

	while ((opt = getopt_long(argc, argv,  sopts, lopts, NULL)) != EOF) {
		switch (opt) {
		case '2':
			i2c = 1;
			break;
		case 'b':
			blockstr = optarg;
			break;
		case 'd':
			device = optarg;
			break;
		case 'e':
			endpoint = strtoul(optarg, NULL, 0);
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

	memset(&command, 0, sizeof(struct i3c_tools_ioctl));
	command.type = I3C_TOOLS_START_BLOCKS;
	command.addr = endpoint;
	if (i2c)
		command.i2cni3c = 1;

	if (send_ioctl(file, &command) < 0)
		goto error;

	/* Process blocks. */

	blocks[i++] = strtok(blockstr, "+");

	do {
		blocks[i++] = strtok(NULL, "+");
	} while (blocks[i - 1]);

	nblocks = i - 1;

	for (i = 0; i < nblocks; ++i) {
		if (i == nblocks - 1) {
			memset(&command, 0, sizeof(struct i3c_tools_ioctl));
			command.type = I3C_TOOLS_LAST_BLOCK;

			if (send_ioctl(file, &command) < 0)
				goto error;
		}

		if ('r' == blocks[i][0]) {
			rc = block_read(file, blocks[i]);
		} else if ('w' == blocks[i][0]) {
			rc = block_write(file, blocks[i]);
		} else {
			fprintf(stderr, "Direction must be r or w!\n");
			goto error;
		}

		if (rc < 0) {
			fprintf(stderr, "Error parsing block!\n");
			goto error;
		}
				   
		if (i == nblocks - 1)
			break;
	}

	memset(&command, 0, sizeof(struct i3c_tools_ioctl));
	command.type = I3C_TOOLS_STOP_BLOCKS;

	if (send_ioctl(file, &command) < 0)
		goto error;

	close(file);

	return 0;

 error:

	if (file)
		close(file);

	return -1;
}
