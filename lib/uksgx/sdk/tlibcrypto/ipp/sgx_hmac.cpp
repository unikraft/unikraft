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

#include "sgx_tcrypto.h"
#include "ippcp.h"
#include "ipp_wrapper.h"
#include "stdlib.h"
#include "string.h"

 /* Message Authentication - HMAC 256
 * Parameters:
 *   Return: sgx_status_t  - SGX_SUCCESS or failure as defined sgx_error.h
 *   Inputs: const unsigned char *p_src - Pointer to input stream to be MACed
 *           int src_len - Source length
 *           const unsigned char *p_key - Pointer to key used in message authentication operation
 *           int key_len - Key length
 *           int mac_len - Expected output MAC length
 *   Output: unsigned char *p_mac - Pointer to resultant MAC
 */
sgx_status_t sgx_hmac_sha256_msg(const unsigned char *p_src, int src_len, const unsigned char *p_key, int key_len,
               unsigned char *p_mac, int mac_len)
{
	if ((p_src == NULL) || (p_key == NULL) || (p_mac == NULL) || (src_len <= 0) || (key_len <= 0) || (mac_len <= 0))  {
		return SGX_ERROR_INVALID_PARAMETER;
	}
	
	sgx_status_t ret = SGX_ERROR_UNEXPECTED;
	IppStatus ipp_ret = ippStsNoErr;

	do {
		ipp_ret = ippsHMACMessage_rmf(p_src, src_len, (const Ipp8u*)p_key, key_len, p_mac, mac_len, ippsHashMethod_SHA256_TT());
		ERROR_BREAK(ipp_ret);

		ret = SGX_SUCCESS;
	} while (0);

	if (ret != SGX_SUCCESS) {
		memset_s(p_mac, mac_len, 0, mac_len); 
	}
	
	return ret;
}

/* Allocates and initializes HMAC state
* Parameters:
*   Return: sgx_status_t  - SGX_SUCCESS or failure as defined in sgx_error.h
*   Inputs: const unsigned char *p_key - Pointer to the key used in message authentication operation
*           int key_len - Key length
*   Output: sgx_hmac_state_handle_t *p_hmac_handle - Pointer to the initialized HMAC state handle
*/
sgx_status_t sgx_hmac256_init(const unsigned char *p_key, int key_len, sgx_hmac_state_handle_t *p_hmac_handle)
{
    if ((p_key == NULL) || (key_len <= 0) || (p_hmac_handle == NULL)) {
        return SGX_ERROR_INVALID_PARAMETER;
    }

	IppStatus ipp_ret = ippStsNoErr;
	sgx_status_t ret = SGX_ERROR_UNEXPECTED;
	IppsHMACState_rmf* pState = NULL;

	int size = 0;

	do {
		ipp_ret = ippsHMACGetSize_rmf(&size);
		ERROR_BREAK(ipp_ret);
		pState = (IppsHMACState_rmf*) malloc(size);
		if (NULL == pState)
		{
			ret = SGX_ERROR_OUT_OF_MEMORY;
			break;
		}
		ipp_ret = ippsHMACInit_rmf(p_key, key_len, pState, ippsHashMethod_SHA256_TT());
		ERROR_BREAK(ipp_ret);

		*p_hmac_handle = pState;
		ret = SGX_SUCCESS;
	} while (0);

	if (ret != SGX_SUCCESS) {
		sgx_hmac256_close((sgx_hmac_state_handle_t)pState);
	}
	
	return ret;
}

/* Updates HMAC hash calculation based on the input message
* Parameters:
*   Return: sgx_status_t  - SGX_SUCCESS or failure as defined in sgx_error.
*	Input:  uint8_t *p_src - Pointer to the input stream to be hashed
*	        int src_len - Length of input stream to be hashed
*	        sgx_hmac_state_handle_t hmac_handle - Handle to the HMAC state
*/
sgx_status_t sgx_hmac256_update(const uint8_t *p_src, int src_len, sgx_hmac_state_handle_t hmac_handle)
{
	if ((p_src == NULL) || (src_len <= 0) || (hmac_handle == NULL)) {
		return SGX_ERROR_INVALID_PARAMETER;
	}
	IppStatus ipp_ret = ippStsNoErr;

	ipp_ret = ippsHMACUpdate_rmf(p_src, (int)src_len, (IppsHMACState_rmf*)hmac_handle);
	if (ipp_ret != ippStsNoErr) {
		return SGX_ERROR_UNEXPECTED;
	}
	return SGX_SUCCESS;
}

/* Returns calculated hash
* Parameters:
*   Return: sgx_status_t  - SGX_SUCCESS or failure as defined in sgx_error.h
*	Input:  sgx_hmac_state_handle_t hmac_handle - Handle to the HMAC state
*	        int hash_len - Expected MAC length
*   Output: unsigned char *p_hash - Resultant hash from HMAC operation
*/
sgx_status_t sgx_hmac256_final(unsigned char *p_hash, int hash_len, sgx_hmac_state_handle_t hmac_handle)
{
    if ((p_hash == NULL) || (hash_len <= 0) || (hmac_handle == NULL)) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
	IppStatus ipp_ret = ippStsNoErr;

	ipp_ret = ippsHMACFinal_rmf(p_hash, hash_len, (IppsHMACState_rmf*)hmac_handle);
	if (ipp_ret != ippStsNoErr) {
		memset_s(p_hash, hash_len, 0, hash_len);
		return SGX_ERROR_UNEXPECTED;
	}

	return SGX_SUCCESS;
}

/* Clean up and free the HMAC state
* Parameters:
*   Return: sgx_status_t  - SGX_SUCCESS or failure as defined in sgx_error.h
*   Input:  sgx_hmac_state_handle_t hmac_handle  - Handle to the HMAC state
* */
sgx_status_t sgx_hmac256_close(sgx_hmac_state_handle_t hmac_handle)
{
    if (hmac_handle == NULL) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
	
    int size = 0;
    IppStatus ipp_ret = ippsHMACGetSize_rmf(&size);
    if (ipp_ret != ippStsNoErr)
    {
        free(hmac_handle);
        return SGX_SUCCESS;
    }

    CLEAR_FREE_MEM(hmac_handle, size);
    return SGX_SUCCESS;
}
