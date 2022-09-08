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

#ifndef __CONFIG_SELECTOR_H__
#define __CONFIG_SELECTOR_H__

#include "tcb_manager.h"
#include "pck_cert_selection.h"
#include "SgxEcdsaAttestation/AttestationParsers.h"


 /**
  * @class ConfigSelect
  * @description valid the tcb and extract the tcb_type value, base on this value to 
  * decompose the CPUSVN, from CPUSVN know the HW type
  */
class ConfigSelect {
public:

	/**
	* Initialize TCBManager, _platformSvn members with input data.
	*/
	ConfigSelect(cpu_svn_t platform_svn,
		const char* tcb_info);
	
	/**
	 * Destructor
	 */
	virtual ~ConfigSelect();

	/**
	* Public class API.
	*
	* Input parsing and verification.
	* and return the corresponding configure id.
	*
	* @param [out] best_cert_index - uint32_t* , the index of selected PCK in the input (construct time) certificates array.
	* @return @ref pck_cert_selection_res_t
	*/
	pck_cert_selection_res_t get_configure_id(uint32_t* config_id);


private:
	// private members
	/**
	 * Platform raw CPUSVN.
	 */
	cpu_svn_t _platformSvn;

	/**
	 * Parsed TCBInfo class.
	*/
	TCBManager _tcbmgr;
	/**
	* must be called before the other function, this function will parse and initialize the tcb
	*/
	pck_cert_selection_res_t parse_input_tcb();


};
#endif //__CONFIG_SELECTOR_H__
