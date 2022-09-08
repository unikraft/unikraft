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
 * @file tcb_manager.h TCBManager class definition.
 */

#ifndef __TCB_MANAGER_H__
#define __TCB_MANAGER_H__

#include "pck_cert_selection.h"
#include "SgxEcdsaAttestation/AttestationParsers.h"
#include <vector>

 /**
  * @class TCBManager
  * @description valid the tcb and extract HW config.
  */
class TCBManager {
public:
	
	/**
	* Initialize platformSvn,tcbInfoString, pemCerts members with input data.
	* pcks, tcbInfo, buckets members are empty at construct.
	* @input tcb_info, the string of tcb_info file
	*/
	TCBManager( const char* tcb_info );
	/**
	 * Destructor
	 */
	virtual ~TCBManager();


	/**
	* Public class API. 
	* intialize TCBInfo format validation
	* must call first
	* @input none
	*/
	pck_cert_selection_res_t tcb_parse_wrapper();
	/**
	* decompose CPUSVN based on tcb_type
	* @input cpusvn raw cpusvn
	* @output res the vector<uint8_t> of decomposed CPUSVN
	*/
	pck_cert_selection_res_t decompose_cpusvn_components(const std::vector<uint8_t>& cpusvn, std::vector<uint8_t>& res);

	/**
	* extract config ID from CPUSVN based on tcb_type
	* @input cpusvn raw cpusvn
	* @output res the config id from CPUSVN
	*/
	pck_cert_selection_res_t extract_config_id_from_cpusvn(const std::vector<uint8_t>& cpusvn, uint32_t &res);

	/**
	* return the parsed tcb_info 
	*/
	intel::sgx::dcap::parser::json::TcbInfo get_tcb_info();
private:
	

	// private members

	/**
	* TCBInfo JSON	input string.
	*/
	const std::string tcbInfoString;

	/**
	 * Parsed TCBInfo class.
	*/
	intel::sgx::dcap::parser::json::TcbInfo tcbInfo;

	/**
	* save the tcb_type that from tcb_info
	*/
	int tcb_type;

	/**
	* The TCP_TYPE is extracted from tcb_info, it will be used to decomple the config ID
	*/
	const unsigned char HW_TYPE = 6;
};
#endif //__TCB_MANAGER_H__
