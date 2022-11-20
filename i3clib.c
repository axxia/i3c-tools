// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 Synopsys, Inc. and/or its affiliates.
 *
 * Author: John Jacques <john.jacques@intel.com>
 */

#include <stdio.h>

void display(const char *header, void *data, int length)
{
	unsigned char *data8 = data;
	unsigned long offset = 0;

        if (header)
                printf("---- %s ----\n", header);

        while (0 < length) {
		char buffer[256];
                int this_line;
		char *string;
		int i;

                string = buffer;
                string += sprintf(string, "%06lx ", offset);
                this_line = (16 > length) ? length : 16;

                for (i = 0; i < this_line; ++i) {
                        string += sprintf(string, "%02x ", *data8++);
                        --length;
                        ++offset;
                }

                printf("%s\n", buffer);
        }
}
