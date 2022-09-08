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

/**
 * @file pck_cert_selection.cpp PCK Cert Selection library interface implementation.
 */

// windows dynamic library / Linux shared object export API
// IMPORTANT: keep this definition before any include to make sure API is exported
#ifdef _WIN32
#define EXPORT_API __declspec(dllexport) 
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "crypt32.lib")
#else
#define EXPORT_API __attribute__ ((visibility("default"))) 
#endif

#include "pck_cert_selection.h"
#include "constants.h"
#include "pck_sorter.h"
#include "config_selector.h"
#include "version.h"


/*
 * Library API function documented in header.
 */
pck_cert_selection_res_t pck_cert_select (
	const cpu_svn_t* platform_svn,
	uint16_t pce_isvsvn,
	uint16_t pce_id,
	const char* tcb_info,
	const char* pem_certs[],
	uint32_t ncerts,
	uint32_t* best_cert_index )
{
	// validate input
	if ( platform_svn == NULL || tcb_info == NULL || pem_certs == NULL || best_cert_index == NULL || ncerts == 0 )
	{
		return PCK_CERT_SELECT_INVALID_ARG;
	}
	PCKSorter pckSorter ( *platform_svn, pce_isvsvn, pce_id, tcb_info, pem_certs, ncerts );
	return pckSorter.select_best_pck ( best_cert_index );
}

pck_cert_selection_res_t platform_sgx_hw_config(
	const cpu_svn_t * platform_svn, 
	const char * tcb_info, 
	uint32_t * configuration_id)
{
	// validate input
	if (platform_svn == NULL || tcb_info == NULL || configuration_id == NULL)
	{
		return PCK_CERT_SELECT_INVALID_ARG;
	}
	ConfigSelect confSelect( *platform_svn, tcb_info);
	return confSelect.get_configure_id(configuration_id);
}

