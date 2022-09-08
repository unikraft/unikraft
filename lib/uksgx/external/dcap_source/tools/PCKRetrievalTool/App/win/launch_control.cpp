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


#include <stdio.h>
#include <windows.h>
#include <WinBase.h>
#include <Enclaveapi.h>
#include <string>
#include <tchar.h>
#include <sgx_urts.h>
#include <utility.h>
#include <sgx_launch_public.h>
#include <metadata.h>
#include <FLC_LE.h>
#include <sgx_enclave_common.h>

#ifndef SGX_SHA256_HASH_SIZE
#define SGX_SHA256_HASH_SIZE            32
#endif
#ifndef SGX_CSS_KEY_MOD_SIZE
#define SGX_CSS_KEY_MOD_SIZE            384
#endif

typedef uint8_t sgx_css_key_modulus_t[SGX_CSS_KEY_MOD_SIZE];


#ifdef DEBUG
#define PRINT_MESSAGE printf
#else
#define PRINT_MESSAGE(...)
#endif

static BOOL WINAPI calMRSigner(sgx_css_key_modulus_t buffer_in, sgx_sha256_hash_t* value)
/*caculate the MRSigner*/
{
	BCRYPT_ALG_HANDLE       hAlg = NULL;
	BCRYPT_HASH_HANDLE      hHash = NULL;
	DWORD                   cbData = 0, cbHash = 0, cbHashObject = 0;
	PBYTE                   pbHashObject = NULL;
	BOOLEAN                 bRet = FALSE;
	NTSTATUS                status = 0;

	if (buffer_in == NULL || value == NULL)
	{
		return FALSE;
	}

	//open an algorithm handle
	if (!BCRYPT_SUCCESS(status = BCryptOpenAlgorithmProvider(
		&hAlg,
		BCRYPT_SHA256_ALGORITHM,
		NULL,
		0)))
	{
		PRINT_MESSAGE("**** Error 0x%x returned by BCryptOpenAlgorithmProvider\n", status);
		goto Cleanup;
	}

	//calculate the size of the buffer to hold the hash object
	if (!BCRYPT_SUCCESS(status = BCryptGetProperty(
		hAlg,
		BCRYPT_OBJECT_LENGTH,
		(PBYTE)&cbHashObject,
		sizeof(DWORD),
		&cbData,
		0)))
	{
		PRINT_MESSAGE("**** Error 0x%x returned by BCryptGetProperty\n", status);
		goto Cleanup;
	}

	//allocate the hash object on the heap
	pbHashObject = (PBYTE)HeapAlloc(GetProcessHeap(), 0, cbHashObject);
	if (NULL == pbHashObject)
	{
		PRINT_MESSAGE("**** memory allocation failed\n");
		goto Cleanup;
	}

	//calculate the length of the hash
	if (!BCRYPT_SUCCESS(status = BCryptGetProperty(
		hAlg,
		BCRYPT_HASH_LENGTH,
		(PBYTE)&cbHash,
		sizeof(DWORD),
		&cbData,
		0)))
	{
		PRINT_MESSAGE("**** Error 0x%x returned by BCryptGetProperty\n", status);
		goto Cleanup;
	}

	//check the hash size
	if (cbHash != SGX_SHA256_HASH_SIZE)
	{
		PRINT_MESSAGE("**** Hash Size is incorrect\n");
		goto Cleanup;
	}

	//create a hash
	if (!BCRYPT_SUCCESS(status = BCryptCreateHash(
		hAlg,
		&hHash,
		pbHashObject,
		cbHashObject,
		NULL,
		0,
		0)))
	{
		PRINT_MESSAGE("**** Error 0x%x returned by BCryptCreateHash\n", status);
		goto Cleanup;
	}

	//hash some data
	if (!BCRYPT_SUCCESS(status = BCryptHashData(
		hHash,
		(PBYTE)buffer_in,
		SGX_CSS_KEY_MOD_SIZE,
		0)))
	{
		PRINT_MESSAGE("**** Error 0x%x returned by BCryptHashData\n", status);
		goto Cleanup;
	}

	//close the hash
	if (!BCRYPT_SUCCESS(status = BCryptFinishHash(
		hHash,
		*value,
		cbHash,
		0)))
	{
		PRINT_MESSAGE("**** Error 0x%x returned by BCryptFinishHash\n", status);
		goto Cleanup;
	}
	bRet = TRUE;
Cleanup:
	if (hAlg)
	{
		BCryptCloseAlgorithmProvider(hAlg, 0);
	}
	if (hHash)
	{
		BCryptDestroyHash(hHash);
	}
	if (pbHashObject)
	{
		HeapFree(GetProcessHeap(), 0, pbHashObject);
	}
	return bRet;
}

// Matches definition of sgx_get_launch_token_func_t, returns enclave_error_t
// Given init info and attributes, calculate and return EINIT token
extern "C"
uint32_t COMM_API sgx_tool_get_launch_token(COMM_IN const enclave_init_sgx_t* p_css, COMM_IN const enclave_sgx_attr_t* p_attr, COMM_OUT enclave_sgx_token_t* p_token)
{
	if (!p_css || !p_attr || !p_token)
		return ENCLAVE_INVALID_PARAMETER;
	
	struct sgx_launch_token_request launch_token_request;
	static void* launch_enclave = NULL;

	// Load the LE.
	// Note that this program will *not* modify IA32_SGXLEPUBKEYHASH MSRs to match the HASH that corresponds to the embedded LE. 
	// Please use an external utility that is able to write the MSRs:
	//   0x93f3e9222f858cc2 0xadd6ecfb29990345 0xacc9da9fa82a9ca6 0x7461236bcdba6a2f	
	if (!launch_enclave)
		launch_enclave = start_launch_enclave();
	if (!launch_enclave)
		return ENCLAVE_UNEXPECTED;

	launch_token_request.version = 0;
	se_static_assert(sizeof(launch_token_request.secs_attr) == sizeof(*p_attr));
	memcpy_s(&launch_token_request.secs_attr, sizeof(launch_token_request.secs_attr), p_attr, sizeof(*p_attr));
	se_static_assert(sizeof(launch_token_request.css) == sizeof(*p_css));
	memcpy_s(&launch_token_request.css, sizeof(launch_token_request.css), p_css, sizeof(*p_css));
	
	struct sgx_launch_request req;
	memset(&req, 0, sizeof(req));
	sgx_sha256_hash_t hash = { 0 };

	uint64_t* tmp = (uint64_t*)launch_token_request.css.key.modulus;
	PRINT_MESSAGE("Hashing %llx ... %llx\n", tmp[0], tmp[47]);

	// Calculate MRSIGNER
	calMRSigner(launch_token_request.css.key.modulus, &hash);

	se_static_assert(SGX_SHA256_HASH_SIZE == sizeof(hash));
	memcpy_s(req.mrsigner, SGX_SHA256_HASH_SIZE, &hash, sizeof(hash));

	se_static_assert(SGX_SHA256_HASH_SIZE == sizeof(launch_token_request.css.body.enclave_hash));
	memcpy_s(req.mrenclave, SGX_SHA256_HASH_SIZE, &launch_token_request.css.body.enclave_hash, sizeof(launch_token_request.css.body.enclave_hash));

	req.attributes = launch_token_request.secs_attr.flags;
	req.xfrm = launch_token_request.secs_attr.xfrm;

	PRINT_MESSAGE("sgx_get_token req.attr = 0x%llx\n", req.attributes);
	PRINT_MESSAGE("sgx_get_token req.xfrm = 0x%llx\n", req.xfrm);
	tmp = (uint64_t*)req.mrenclave;
	PRINT_MESSAGE("sgx_get_token req.mrenclave = 0x%llx 0x%llx\n", tmp[0], tmp[3]);
	tmp = (uint64_t*)req.mrsigner;
	PRINT_MESSAGE("sgx_get_token req.mrsigner = 0x%llx 0x%llx\n", tmp[0], tmp[3]);

	// Ask the LE to get a launch token
	__try
	{
		req.output.result = -1;
		sgx_get_token(&req, launch_enclave);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return ENCLAVE_UNEXPECTED;
	}
	if (req.output.result)
	{
		PRINT_MESSAGE("Assembly function sgx_get_token() failed\n");
	}

	memset(p_token, 0, sizeof(*p_token));
	se_static_assert(sizeof(*p_token) >= sizeof(req.output.token));
	memcpy_s(p_token, sizeof(*p_token), &req.output.token, sizeof(req.output.token));
	return ENCLAVE_ERROR_SUCCESS;
}

