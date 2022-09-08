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
// Jarkko Sakkinen <jarkko.sakkinen@linux.intel.com>

#include <stdint.h>
#include "asm/sgx_arch.h"
#include "mbedtls/cmac.h"
#include "main.h"
#include "string.h"
#include "whitelist.h"

#define SGX_FLAGS_PROVISION_KEY  0x0000000000000010ULL
#define __aligned(x) __attribute__ ((aligned (x)))

struct sgx_le_output {
	struct sgx_einittoken token;
	int result;
};

struct sgx_launch_request {
	uint8_t mrenclave[32];
	uint8_t mrsigner[32];
	uint64_t attributes;
	uint64_t xfrm;
	struct sgx_le_output output;
};

#define SGX_SUCCESS			0
#define SGX_INVALID_ATTRIBUTE	2

#define SGX_INVALID_PRIVILEGE	0x40000002
#define SGX_UNEXPECTED_ERROR	0x40000003


static bool rdrand_uint32(uint32_t *value)
{
	int i;

	for (i = 0; i < RAND_NR_TRIES; i++) {
		if (__builtin_ia32_rdrand32_step((unsigned int *)value))
			return true;
	}

	return false;
}

static int sign_einittoken(struct sgx_einittoken *einittoken)
{
	struct sgx_keyrequest keyrequest __aligned(512);
	uint8_t launch_key[16] __aligned(16);
	uint32_t *keyid_ptr;
	int i;
	int ret;


	/* Despite its misleading name, the only purpose of the keyid field is
	 * to add entropy to the token so that every token will have an unique
	 * CMAC.
	 */
	keyid_ptr = (uint32_t *)einittoken->keyid;

	for (i = 0; i < sizeof(einittoken->keyid) / 4; i++)
		if (!rdrand_uint32(&keyid_ptr[i]))
			return SGX_UNEXPECTED_ERROR;

	memset(&keyrequest, 0, sizeof(keyrequest));
	keyrequest.keyname = 0; /* LICENSE_KEY */
	memcpy(&keyrequest.keyid, &einittoken->keyid, sizeof(keyrequest.keyid));
	memcpy(&keyrequest.cpusvn, &(einittoken->cpusvnle),
	       sizeof(keyrequest.cpusvn));
	memcpy(&keyrequest.isvsvn, &(einittoken->isvsvnle),
	       sizeof(keyrequest.isvsvn));

	keyrequest.attributemask = ~SGX_ATTR_MODE64BIT;
	keyrequest.xfrmmask = 0;
	keyrequest.miscmask = 0xFFFFFFFF;

	einittoken->maskedmiscselectle &= keyrequest.miscmask;
	einittoken->maskedattributesle &= keyrequest.attributemask;
	einittoken->maskedxfrmle &= keyrequest.xfrmmask;

	if (sgx_egetkey(&keyrequest, launch_key))
		return false;

	ret = mbedtls_cipher_cmac(launch_key, (const unsigned char *)&einittoken->payload, sizeof(einittoken->payload), (unsigned char *)einittoken->mac);

	memset(launch_key, 0, sizeof(launch_key));

	return ret ? SGX_UNEXPECTED_ERROR : SGX_SUCCESS;
}

static int create_einittoken(uint8_t *mrenclave,
			      uint8_t *mrsigner,
			      uint64_t attributes,
			      uint64_t xfrm,
			      struct sgx_einittoken *einittoken)
{

	struct sgx_targetinfo tginfo __aligned(512);
	struct sgx_report report __aligned(512);
	uint8_t reportdata[64] __aligned(128);

	if (attributes & SGX_ATTR_RESERVED_MASK)
		return SGX_INVALID_ATTRIBUTE;

	memset(&tginfo, 0, sizeof(tginfo));
	memset(reportdata, 0, sizeof(reportdata));
	memset(&report, 0, sizeof(report));

	if (sgx_ereport(&tginfo, reportdata, &report))
		return SGX_UNEXPECTED_ERROR;

	memset(einittoken, 0, sizeof(*einittoken));

	einittoken->payload.valid = 1;

	memcpy(einittoken->payload.mrenclave, mrenclave, 32);
	memcpy(einittoken->payload.mrsigner, mrsigner, 32);
	einittoken->payload.attributes = attributes;
	einittoken->payload.xfrm = xfrm;

	memcpy(&einittoken->cpusvnle, &report.cpusvn,
		   sizeof(report.cpusvn));
	einittoken->isvsvnle = report.isvsvn;
	einittoken->isvprodidle = report.isvprodid;

	einittoken->maskedattributesle = report.attributes;
	einittoken->maskedxfrmle = report.xfrm;
	einittoken->maskedmiscselectle = report.miscselect;

	return sign_einittoken(einittoken);
}

void encl_body(struct sgx_launch_request *req)
{
	struct sgx_einittoken token;
	uint8_t mrenclave[32];
	uint8_t mrsigner[32];
	uint64_t attributes;
	uint64_t xfrm;
	int i;

	if (!req)
		return;

	memcpy(mrenclave, req->mrenclave, sizeof(mrenclave));
	memcpy(mrsigner, req->mrsigner, sizeof(mrsigner));
	memcpy(&attributes, &req->attributes, sizeof(uint64_t));
	memcpy(&xfrm, &req->xfrm, sizeof(uint64_t));
	memset(&token, 0, sizeof(token));

	if((attributes & SGX_FLAGS_PROVISION_KEY)) {
		for(i = 0; i < (sizeof(G_SERVICE_ENCLAVE_MRSIGNER) / sizeof(G_SERVICE_ENCLAVE_MRSIGNER[0])); i++) {
			if(0 == memcmp(&G_SERVICE_ENCLAVE_MRSIGNER[i], mrsigner, sizeof(G_SERVICE_ENCLAVE_MRSIGNER[0])))
				break;
		}
		if(i == sizeof(G_SERVICE_ENCLAVE_MRSIGNER) / sizeof(G_SERVICE_ENCLAVE_MRSIGNER[0])) {
			req->output.token.payload.valid = 0;
            req->output.result = SGX_INVALID_PRIVILEGE;
			return;
		}
	}

	req->output.result = create_einittoken(mrenclave, mrsigner, attributes, xfrm, &token);

	if (req->output.result == SGX_SUCCESS)
		memcpy(&req->output.token, &token, sizeof(token));
}
