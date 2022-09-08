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
 * @mainpage PCK Cert Selection Library
 *
 * @section Intro
 * PCK Cert Selection library enables a platform owner to choose from a cached PCK Cert list the PCK with highest TCB 
 * for the platform and use it.
 * PCK Cert Selection library enables a platform owner to retrieve the HW type configuration representation in the CPUSVN.
 *
 * @section API
 * The library exposes two function API
 *
 * @ref pck_cert_select() enables the user to select the 'best' PCK to use for a platform attestation.
 *
 * The input is the raw platform CPU SVN and PCE ISV SVN along with a TCBInfo structure in JSON format and a list of
 * cached PCK Certs for the platform.
 *
 * The output is the index of the 'best" PCK in the input PCK Cert list.
 *
 * @ref platform_sgx_hw_config() enables the user to retrieve the configuration representation in the CPUSVN
 *
 * The input is the raw platform CPU SVN and a TCBInfo structure in JSON format 
 *
 * The output is the HW type configuration representation in the CPUSVN 
 *
 * @section Implementation
 * The library API is implemented in pck_cert_selection.cpp.
 * All logic is implemented in class @ref PCKSorter, @ref TCBManager .
 */

/**
 * @file pck_cert_selection.h
 * @description Defines PCK Cert Selection library interface.
 */




#ifndef _PCK_CERT_SELECTION_H_
#define _PCK_CERT_SELECTION_H_

#include <stdint.h>

/**
 * DLL export macro
 */
#ifndef EXPORT_API
#ifdef _WIN32
#define EXPORT_API __declspec(dllimport)
#else
#define EXPORT_API 
#endif // _WIN32
#endif // EXPORT_API


#ifdef __cplusplus
extern "C" {
#endif

// keep structures packed
#pragma pack (push , 1)

/**
 * CPUSVN size
 */
#define CPUSVN_SIZE		(16)

/**
 * CPUSVN -	CPU security version number blob.
 * @size 16 bytes. 
 */
typedef struct
{
	uint8_t bytes[CPUSVN_SIZE];
} cpu_svn_t;

#pragma pack (pop)


/**
 * PCK Cert Selection Library return values
 */
typedef enum
{
	PCK_CERT_SELECT_SUCCESS					= 0,	/**< OK														*/
	PCK_CERT_SELECT_INVALID_ARG				= 1,	/**< Invalid argument										*/
	PCK_CERT_SELECT_INVALID_CERT			= 2,	/**< Invalid PCK certificate								*/
	PCK_CERT_SELECT_INVALID_CERT_CPUSVN		= 3,	/**< PCK certificate CPUSVN doesn't match TCB Components	*/
	PCK_CERT_SELECT_INVALID_CERT_VERSION	= 4,	/**< Invalid PCK certificate version						*/
	PCK_CERT_SELECT_UNEXPECTED				= 5,	/**< Unexpected error										*/
	PCK_CERT_SELECT_INVALID_PCK_PCEID		= 6,	/**< PCK PCE ID doesn't match input PCE ID					*/
	PCK_CERT_SELECT_INVALID_PPID			= 7,	/**< PCKs PPID doesn't match	other PCKs					*/
	PCK_CERT_SELECT_INVALID_FMSPC			= 8,	/**< PCKs FMSPC doesn't match other PCKs					*/
	PCK_CERT_SELECT_INVALID_TCB				= 9,	/**< Invalid TCB Info 										*/
	PCK_CERT_SELECT_INVALID_TCB_PCEID		= 10,	/**< TCB Info PCE ID doesn't match input PCE ID				*/
	PCK_CERT_SELECT_UNSUPPORTED_TCB_TYPE	= 11,	/**< TCB Info TCB Type not supported						*/
	PCK_CERT_SELECT_PCK_NOT_FOUND			= 12,	/**< Raw TCB is lower than all input PCKs					*/
} pck_cert_selection_res_t;


/**
 * Compare raw TCB (CPUSVN and PCESVN) with input PCKs TCB.
 * Find 'best' PCK - the PCK with highest TCB which is lower than raw TCB.
 * 
 * @note Input PCKs and TCBInfo structures are validated but it's signature and validity (Not Before - Not After) are not verified.
 *		 It is client responsibility to verify the input.
 *
 * @param [in]  platform_svn	- const cpu_svn_t*, Raw platform CPUSVN, can't be NULL.
 * @param [in]  pce_isvsvn		- uint16_t, Raw platform PCE ISV SVN.
 * @param [in]  pce_id			- uint16_t, Raw platform PCE ID.
 * @param [in]  tcb_info		- const char*, Platform TCBInfo structure in JSON format, can't be NULL.
 * @param [in]  pem_certs[]		- const char*, Array of NULL terminated Intel SGX PCK certificates in PEM format, can't be NULL, array members can't be NULL.
 * @param [in]  ncerts			- uint32_t, Size of pem_certs array, can't be 0.
 * @param [out] best_cert_index - uint32_t*, Index of best PCK in pem_certs array, can't be NULL, valid only if function returns PCK_CERT_SELECT_SUCCESS.
 *
 * @return @ref pck_cert_selection_res_t
 *	- @ref PCK_CERT_SELECT_SUCCESS - found matching PCK, the PCK index is returned in best_cert_index.
 *	- @ref PCK_CERT_SELECT_PCK_NOT_FOUND - raw TCB is lower than all input PCKs.
 * Error occurred:
 *	- @ref PCK_CERT_SELECT_INVALID_ARG
 *	- @ref PCK_CERT_SELECT_INVALID_CERT
 *	- @ref PCK_CERT_SELECT_INVALID_CERT_CPUSVN
 *	- @ref PCK_CERT_SELECT_INVALID_CERT_VERSION
 *	- @ref PCK_CERT_SELECT_UNEXPECTED
 *	- @ref PCK_CERT_SELECT_INVALID_PCK_PCEID
 *	- @ref PCK_CERT_SELECT_INVALID_PPID
 *	- @ref PCK_CERT_SELECT_INVALID_FMSPC
 *	- @ref PCK_CERT_SELECT_INVALID_TCB
 *	- @ref PCK_CERT_SELECT_INVALID_TCB_PCEID
 *	- @ref PCK_CERT_SELECT_UNSUPPORTED_TCB_TYPE
 */
EXPORT_API pck_cert_selection_res_t pck_cert_select (
	const cpu_svn_t* platform_svn,
	uint16_t pce_isvsvn, 
	uint16_t pce_id, 
	const char* tcb_info, 
	const char* pem_certs[], 
	uint32_t ncerts, 
	uint32_t* best_cert_index );

/**
 * Extrace HW configuration from CPUSVN.
 * @note Input TCBInfo structures are validated but it's signature and validity (Not Before - Not After) are not verified.
 *		 It is client responsibility to verify the input.
 *
 * @param [in]  platform_svn	 - const cpu_svn_t*, Raw platform CPUSVN, can't be NULL.
 * @param [in]  tcb_info		- const char*, Platform TCBInfo structure in JSON format, can't be NULL.
 * @param [out] configuration_id - uint32_t*, the configuration id, can't be NULL, valid only if function returns PCK_CERT_SELECT_SUCCESS.
 */
EXPORT_API pck_cert_selection_res_t platform_sgx_hw_config(
	const cpu_svn_t* platform_svn,
	const char* tcb_info,
	uint32_t* configuration_id);
#ifdef __cplusplus
}
#endif

#endif	// _PCK_CERT_SELECTION_H_
