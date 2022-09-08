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

#include "constants.h"
#include "config_selector.h"
#include "pck_cert_selection.h"
#include "pck_sorter.h"
using namespace intel::sgx::dcap::parser::json;

#include <vector>
#include <algorithm>
using namespace std;

/*
 * Public constructor documented in header.
 */
ConfigSelect::ConfigSelect(cpu_svn_t platform_svn,
	const char * tcb_info)
	: _platformSvn(platform_svn),
	_tcbmgr(tcb_info)
{	
}

/*
 * The initialize function will do a basic check for tcb
 */

pck_cert_selection_res_t ConfigSelect::parse_input_tcb()
{
	pck_cert_selection_res_t ret = PCK_CERT_SELECT_SUCCESS;
	ret =  _tcbmgr.tcb_parse_wrapper();
	return ret;
}
// destructor
ConfigSelect::~ConfigSelect()
{
}

pck_cert_selection_res_t ConfigSelect::get_configure_id(uint32_t * config_id)
{
	pck_cert_selection_res_t ret = PCK_CERT_SELECT_SUCCESS;
	vector<uint8_t> tcb_decomponents;
	uint32_t conf_id = INT32_MAX;

	ret = parse_input_tcb();
	if (ret != PCK_CERT_SELECT_SUCCESS)
		return ret;

	// read raw CPUSVN into vector and decompose to TCB Components
	const vector<uint8_t> rawCPUSVN(_platformSvn.bytes, _platformSvn.bytes + CPUSVN_SIZE);
	/* It is important that the code clearly states the the CPUSVN is not architecturally 
	*defined and can change from platform to platform based on its TCBType.
	*/
	ret = _tcbmgr.extract_config_id_from_cpusvn(rawCPUSVN, conf_id);
	if (ret == PCK_CERT_SELECT_SUCCESS)
		*config_id = conf_id;
	return ret;
}

