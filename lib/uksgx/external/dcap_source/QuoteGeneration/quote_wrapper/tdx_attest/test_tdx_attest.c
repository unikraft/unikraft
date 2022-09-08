/*
 * Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "tdx_attest.h"

#define devname		"/dev/tdx-attest"

#define HEX_DUMP_SIZE	16
#define MAX_ROW_SIZE	70

static void print_hex_dump(const char *title, const char *prefix_str,
		const uint8_t *buf, int len)
{
	const uint8_t *ptr = buf;
	int i, rowsize = HEX_DUMP_SIZE;

	if (!len || !buf)
		return;

	fprintf(stdout, "\t\t%s", title);

	for (i = 0; i < len; i++) {
		if (!(i % rowsize))
			fprintf(stdout, "\n%s%.8x:", prefix_str, i);
		if (ptr[i] <= 0x0f)
			fprintf(stdout, " 0%x", ptr[i]);
		else
			fprintf(stdout, " %x", ptr[i]);
	}

	fprintf(stdout, "\n");
}

void gen_report_data(uint8_t *reportdata)
{
	int i;

	srand(time(NULL));

	for (i = 0; i < TDX_REPORT_DATA_SIZE; i++)
		reportdata[i] = rand();
}

int main(int argc, char *argv[])
{
    uint32_t quote_size = 0;
    tdx_report_data_t report_data = {{0}};
    tdx_report_t tdx_report = {{0}};
    tdx_uuid_t selected_att_key_id = {0};
    uint8_t *p_quote_buf = NULL;
    FILE *fptr = NULL;

    gen_report_data(report_data.d);
    print_hex_dump("\n\t\tTDX report data\n", " ", report_data.d, sizeof(report_data.d));

    if (TDX_ATTEST_SUCCESS != tdx_att_get_report(&report_data, &tdx_report)) {
        fprintf(stderr, "\nFailed to get the report\n");
        return 1;
    }
    print_hex_dump("\n\t\tTDX report\n", " ", tdx_report.d, sizeof(tdx_report.d));

    if (TDX_ATTEST_SUCCESS != tdx_att_get_quote(&report_data, NULL, 0, &selected_att_key_id,
        &p_quote_buf, &quote_size, 0)) {
        fprintf(stderr, "\nFailed to get the quote\n");
        return 1;
    }
    print_hex_dump("\n\t\tTDX quote data\n", " ", p_quote_buf, quote_size);

    fprintf(stdout, "\nSuccessfully get the TD Quote\n");
    fptr = fopen("quote.dat","wb");
    if( fptr )
    {
        fwrite(p_quote_buf, quote_size, 1, fptr);
        fclose(fptr);
    }
    fprintf(stdout, "\nWrote TD Quote to quote.dat\n");

    tdx_att_free_quote(p_quote_buf);
    return 0;
}
