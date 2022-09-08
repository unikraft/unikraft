// This file is provided under a dual BSD/GPLv2 license.  When using or
// redistributing this file, you may do so under either license.
//
// GPL LICENSE SUMMARY
//
// Copyright(c) 2016-2018 Intel Corporation.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of version 2 of the GNU General Public License as
// published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact Information:
// Jarkko Sakkinen <jarkko.sakkinen@linux.intel.com>
// Intel Finland Oy - BIC 0357606-4 - Westendinkatu 7, 02160 Espoo
//
// BSD LICENSE
//
// Copyright(c) 2016-2018 Intel Corporation.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//   * Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in
//     the documentation and/or other materials provided with the
//     distribution.
//   * Neither the name of Intel Corporation nor the names of its
//     contributors may be used to endorse or promote products derived
//     from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Authors:
//
// Haim Cohen <haim.cohen@intel.com>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <ctype.h>
#include <sys/stat.h>


static void exit_usage(const char *program)
{
	fprintf(stderr,
		"Usage: %s <bin_file> <header_name> <array_name>\n", program);
	exit(1);
}

static void print_lic(FILE *output) {
	fprintf(output, "/**\n");
	fprintf(output, "Copyright (C) 2011-2019 Intel Corporation. All rights reserved.\n\n");
	fprintf(output, "Redistribution and use in source and binary forms, with or without\n");
	fprintf(output, "modification, are permitted provided that the following conditions\n");
	fprintf(output, "are met:\n\n");
	fprintf(output, "  * Redistributions of source code must retain the above copyright\n");
	fprintf(output, "    notice, this list of conditions and the following disclaimer.\n");
	fprintf(output, "  * Redistributions in binary form must reproduce the above copyright\n");
	fprintf(output, "    notice, this list of conditions and the following disclaimer in\n");
	fprintf(output, "    the documentation and/or other materials provided with the\n");
	fprintf(output, "    distribution.\n");
	fprintf(output, "  * Neither the name of Intel Corporation nor the names of its\n");
	fprintf(output, "    contributors may be used to endorse or promote products derived\n");
	fprintf(output, "    from this software without specific prior written permission.\n\n");
	fprintf(output, "THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n");
	fprintf(output, "\"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n");
	fprintf(output, "LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR\n");
	fprintf(output, "A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT\n");
	fprintf(output, "OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,\n");
	fprintf(output, "SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT\n");
	fprintf(output, "LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,\n");
	fprintf(output, "DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY\n");
	fprintf(output, "THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\n");
	fprintf(output, "(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n");
	fprintf(output, "OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n");
	fprintf(output, "**/\n\n");
}


int main(int argc, char **argv)
{
	const char *program;
	int opt;
    FILE *input = NULL;
    FILE *output = NULL;
	int res = 0;
    int offset, i;
	uint8_t data[0x1000];
    long bytes_read = 0;
	struct stat sb;
	char upper_name[256];
	char *s, *p;

	program = argv[0];

	do {
		opt = getopt(argc, argv, "");
		switch (opt) {
		case -1:
			break;
		default:
			exit_usage(program);
		}
	} while (opt != -1);

	argc -= optind;
	argv += optind;

	if (argc < 3)
		exit_usage(program);

    output = fopen(argv[1], "w");
	if (!output) {
		perror("fopen");
		goto out;
	}

	input = fopen(argv[0], "rb");
	if (!input) {
		perror("fopen");
		goto out;
	}

	res = stat(argv[0], &sb);
	if (res) {
		perror("stat");
		goto out;
	}

	s = argv[2];
	p = upper_name;
	while (*s) {
		*p = toupper(*s);
		s++; p++;
	}
	*p = 0;

	print_lic(output);

	fprintf(output, "#pragma once\n\n");

	fprintf(output, "#define %s_LEN %ld\n", upper_name, sb.st_size);
	fprintf(output, "const int %s_length = %s_LEN;\n\n", argv[2], upper_name);

	fprintf(output, "const unsigned char %s[%s_LEN] = \n{", argv[2], upper_name);

	for (offset = 0; offset < sb.st_size; offset += 0x1000) {

		int rc = fread(data, 1, 0x1000, input);
		if (!rc)
			break;

        bytes_read += rc;

        for (i = 0; i < rc; ++i)
        {
            if ((i % 16) == 0)
                fprintf(output, "\n\t");
            fprintf(output, "0x%.2x,", data[i] & 0xff);
        }
	}

	fprintf(output, "\n};\n");

    if (bytes_read < sb.st_size)
        res = 1;

out:
    if (input)
        fclose(input);
    if (output)
        fclose(output);
    return res;
}

