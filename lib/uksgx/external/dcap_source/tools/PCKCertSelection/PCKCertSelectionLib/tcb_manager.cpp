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
#include "tcb_manager.h"
#include "pck_sorter.h"
using namespace intel::sgx::dcap::parser::json;

#include <vector>
#include <algorithm>
using namespace std;

/*
 * Public constructor documented in header.
 */
TCBManager::TCBManager( const char * tcb_info)
	: tcbInfoString(tcb_info),
	tcb_type(0)
{
}

// destructor
TCBManager::~TCBManager()
{
}



/**
 * Initialize the TCB will do a general check.
 *
 * @param[in] none
 * @param[out] will return success if no issue found.
 *
 */
pck_cert_selection_res_t TCBManager::tcb_parse_wrapper()
{
	// general TCBInfo format validation
	try
	{
		this->tcbInfo = TcbInfo::parse(this->tcbInfoString);
	}
	catch (const std::exception&)
	{
		return PCK_CERT_SELECT_INVALID_TCB;
	}

	// TCB Type check, currently only type 0 is supported
	// TCbInfo::v1 was unsupported and current only TcbType::v2 and 
	// TcbType::v3 are supported

	if ( this->tcbInfo.getVersion () != static_cast<unsigned int>( TcbInfo::Version::V2 ) &&
      this->tcbInfo.getVersion () != static_cast<unsigned int>( TcbInfo::Version::V3 ) )

	{
		   return PCK_CERT_SELECT_INVALID_TCB;
    }

	this->tcb_type = this->tcbInfo.getTcbType();
	if (this->tcb_type != 0)
	{
		return PCK_CERT_SELECT_UNSUPPORTED_TCB_TYPE;
	}
	return PCK_CERT_SELECT_SUCCESS;
}

/**
 * Decompose raw platform CPUSVN to TCB Components vector based on TCBType algorithm.
 * @note the composition of raw platform CPUSVN is not architecturally defined and its composition can change with the release of a new TCBInfo for any given FMSPC.
 *
 * @param[in] cpusvn - const vector<uint8_t>& , Input raw CPUSVN in byte vector format.
 * @param[out] res - A vector of TCB Components decomposition of CPUSVN.
 * 
 */
pck_cert_selection_res_t TCBManager::decompose_cpusvn_components(const std::vector<uint8_t>& cpusvn, std::vector<uint8_t>& res)
{

	// read raw CPUSVN into vector and decompose to TCB Components
	// the CPUSVN composition for HW is not architecturally defined and can change in future products
	// the tcb_type field in the TCBInfo indicates the HW composition for this HW platform

	if (tcb_type == 0)
	{
		// for TCB Type 0 it is simple copy
		res = cpusvn;
		return PCK_CERT_SELECT_SUCCESS;
	}
	//currently only type 0 is supported
	return PCK_CERT_SELECT_UNSUPPORTED_TCB_TYPE;
}

/**
 * extract config ID from CPUSVN based on tcb_type
 * @note the composition of raw platform CPUSVN is not architecturally defined and its composition can change with the release of a new TCBInfo for any given FMSPC.
 *
 * @param[in] cpusvn - const vector<uint8_t>& , Input raw CPUSVN in byte vector format.
 * @param[out] res - A uint32_t of config id that extracted from raw CPUSVN based on tcb_type.
 */
pck_cert_selection_res_t TCBManager::extract_config_id_from_cpusvn(const std::vector<uint8_t>& cpusvn, uint32_t & res)
{
	// Be clear in the comments that this is only correct for TCBType = 0.
	if (tcb_type == 0)
	{
		//For all TCBTypes == 0, the SGX HW configuration is located in byte[6] of the platform's CPUSVN.
		res = cpusvn[HW_TYPE];
		return PCK_CERT_SELECT_SUCCESS;
	}
	//currently only type 0 is supported
	return PCK_CERT_SELECT_UNSUPPORTED_TCB_TYPE;
}

intel::sgx::dcap::parser::json::TcbInfo TCBManager::get_tcb_info()
{
	return tcbInfo;
}

