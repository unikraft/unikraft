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
 * @file pck_sorter.cpp PCKSorter class implementation
 */

#include "constants.h"
#include "pck_sorter.h"
using namespace intel::sgx::dcap::parser::x509;
using namespace intel::sgx::dcap::parser::json;

#include <algorithm>
using namespace std;


/*
 * Public constructor documented in header.
 */
PCKSorter::PCKSorter ( cpu_svn_t platform_svn,
					   uint16_t pce_isvsvn,
					   uint16_t pce_id,
					   const char* tcb_info,
					   const char* pem_certs[],
					   uint32_t ncerts )
	: platformSvn ( platform_svn ),
	pceIsvSvn ( pce_isvsvn ),
	pceID ( pce_id ),
	tcbmgr(tcb_info),
	pemCerts ( pem_certs, pem_certs + ncerts ),
	pcks {},
	tcbInfo {},
	buckets {}
{
}


// destructor
PCKSorter::~PCKSorter ()
{
}


/**
 * Helper function for parse_input_tcb_and_pcks.
 * Empty pcks container and return the input result.
 * @param [in] res - pck_cert_selection_res_t, The result to return.
 * @return @ref pck_cert_selection_res_t
 */
pck_cert_selection_res_t PCKSorter::clean_pcks_return ( pck_cert_selection_res_t res )
{
	this->pcks.clear ();
	return res;
}


/**
 * Parse and validate the input TCBInfo and PCK Certs.
 *
 * -# Read input TCBInfo string into TCBInfoJsonVerifier and validate:
 *	-# Version.
 *	-# General format, TCB components exist.
 *	-# TCB type supported - must be 0 for first version, optional field default to 0.
 *
 * -# Read PCKS strings into CertStore and validate.
 *	  Parse and verify certificates - no signature verification.
 *	  Parsed valid certificates are inserted to pcks container.
 *	-# PCK is not NULL.
 *	-# General format checks.
 *	-# TCB components exist.
 *		-# CPUSVN decomposition match TCB Components 1..16.
 *	-# Check version.
 *	-# PCK.PCEID equals input pce_id.
 *	-# PCK.FMSPC equals input TCBInfo.FMSPC.
 *	-# Verify all Certs have the same PPID.
 *	-# Insert validated PCK to container.
 *
 * @pre Object initialized with raw input.
 * @post On success, pcks container holds parsed validated certificates.
 *		On failure pcks container is empty.
 *
 * @return @ref pck_cert_selection_res_t
 */
pck_cert_selection_res_t PCKSorter::parse_input_tcb_and_pcks ( void )
{
	// general TCBInfo format validation
	pck_cert_selection_res_t ret = PCK_CERT_SELECT_SUCCESS;
	ret = parse_input_tcb();
	if (ret != PCK_CERT_SELECT_SUCCESS)
		return ret;

	// PCEID in JSON is big BE, convert to LE and compare with input PCEID
	int16_t tcbPCEID = (this->tcbInfo.getPceId ()[0] << 8) + this->tcbInfo.getPceId ()[1];
	if ( tcbPCEID != this->pceID )
	{
		return PCK_CERT_SELECT_INVALID_TCB_PCEID;
	}

	// PCKs parsing and validation
	this->pcks.reserve ( this->pemCerts.size () );

	// hold PPID of first PCK Cert, to compare with all Certs
	vector<uint8_t> ppid;

	// parse and verify certificates
	for ( uint32_t i = 0; i < this->pemCerts.size (); i++ )
	{
		// 1. pck is not NULL
		if ( this->pemCerts[i] == NULL )
		{
			return clean_pcks_return ( PCK_CERT_SELECT_INVALID_ARG );
		}

		// 2. general format checks - using QVL parser
		auto pckCert = make_shared < PckCertificate > ();
		try
		{
			*pckCert = PckCertificate::parse ( this->pemCerts[i] );
		}
		catch ( const std::exception& )
		{
			return clean_pcks_return ( PCK_CERT_SELECT_INVALID_CERT );
		}

		// 3. check PCK version
		if ( pckCert->getVersion () != PCK_CERT_VERSION )
		{
			return clean_pcks_return ( PCK_CERT_SELECT_INVALID_CERT_VERSION );
		}

		// 4. verify CPUSVN match TCB Components
		// in depth verification, valid PCK are expected to have match
		// decompose CPUSVN based on algorithm (TCBType) and compare to TCB Components
		vector < uint8_t > components;
		this->tcbmgr.decompose_cpusvn_components(pckCert->getTcb().getCpuSvn(), components);
		if ( this->equal_bytes ( components, pckCert->getTcb().getSgxTcbComponents () ) == false )
		{
			return clean_pcks_return ( PCK_CERT_SELECT_INVALID_CERT_CPUSVN );
		}

		// the reason PCEID, PPID and FMSPC are extracted here and not in PckCertificate parsing
		// is that these fields are only used here for PCK validation, and not used after that for sort

		// 5. PCK.PCEID equals input pce_id
		// get PCE-ID SGXExtension value and compare to input pce_id 

		// check PCE ID value
		vector<uint8_t> pceid = pckCert->getPceId ();
		int16_t pckPCEID = ( pceid[0] << 8 ) + pceid[1];
		if ( this->pceID != pckPCEID )
		{
			return clean_pcks_return ( PCK_CERT_SELECT_INVALID_PCK_PCEID );
		}

		// 6. find FMSPC SGX extension using lambda function search

		// check FMSPC value
		if ( this->equal_bytes ( this->tcbInfo.getFmspc (), pckCert->getFmspc () ) == false )
		{
			return clean_pcks_return ( PCK_CERT_SELECT_INVALID_FMSPC );
		}

		// 7. find PPID SGX extension using lambda function search

		// save PPID of first PCK Cert
		// compare all other certs values
		if ( i == 0 )
		{
			ppid = pckCert->getPpid ();
		}
		else
		{
			if ( this->equal_bytes ( ppid, pckCert->getPpid () ) == false )
			{
				return clean_pcks_return ( PCK_CERT_SELECT_INVALID_PPID );
			}
		}

		// 8. input cert validated, insert to pcks container
		this->pcks.emplace_back ( pckCert );
	}

	return PCK_CERT_SELECT_SUCCESS;
}


/**
 * Compare two bytes vectors content.
 *
 * @param left	- const vector < uint8_t >&, Left byte vector.
 * @param right	- const vector < uint8_t >&, Right byte vector.
 * @return
 * @b true - If vectors are of same size and all bytes of left are equal to matching bytes of right.
 * @b false - Otherwise.
 */
bool PCKSorter::equal_bytes ( const vector < uint8_t >& left, const vector < uint8_t >& right )
{
	if ( left.size () != right.size () )
	{
		return false;
	}
	for ( int i = 0; i < left.size (); i++ )
	{
		if ( left[i] != right[i] )
		{
			return false;
		}
	}
	return true;
}


/**
 * Compare two TCB Components vectors, byte by byte and 2 PCESVNs.
 * 
 * @param[in] left			- const vector<uint8_t>&, Left TCB components raw vector.
 * @param[in] left_pcesvn	- int64_t, Left PCESVN.
 * @param[in] right			- const vector<uint8_t>&, Right TCB components raw vector.
 * @param[in] right_pcesvn	- int64_t, Right PCESVN.
 * @return @ref comp_res_t
 */
PCKSorter::comp_res_t PCKSorter::compare_tcb_components ( const vector<uint8_t>& left, int64_t left_pcesvn, const vector<uint8_t>& right, int64_t right_pcesvn )
{
	if ( left.size () != CPUSVN_SIZE || right.size () != CPUSVN_SIZE )
	{
		return comp_res_t::COMP_ERROR;
	}

	bool left_lower = false, right_lower = false;

	// compare PCESVNs
	if ( left_pcesvn < right_pcesvn )
	{
		left_lower = true;
	}
	if ( left_pcesvn > right_pcesvn )
	{
		right_lower = true;
	}

	// compare components, byte by byte
	for ( size_t i = 0; i < CPUSVN_SIZE; i++ )
	{
		if ( left[i] < right[i] )
		{
			left_lower = true;
		}
		if ( left[i] > right[i] )
		{
			right_lower = true;
		}
	}

	if ( left_lower && right_lower )
	{
		return comp_res_t::COMP_UNDEFINED;
	}
	if ( left_lower )
	{
		return comp_res_t::COMP_LOWER;
	}
	return comp_res_t::COMP_EQUAL_OR_GREATER;
}


/**
 * Sort PCK Certs to buckets.
 * TCBInfo TCB Levels are carefully sorted by Intel.
 * Create a bucket for every TCB Level + one additional bucket for 
 * PCKs that doesn't match any TCB Level.
 * Two stages sort:
 * 1. Add PCK to first bucket where PCK TCB is greater or equal to bucket TCB Level.
 * 2. When insert PCK to a bucket, keep bucket sorted, by compare the new PCK TCB to PCKs in bucket
 *    hence first PCK in bucket always has greatest TCB.
 */
void PCKSorter::sort_to_buckets ( void )
{
	// number of buckets is number of TCBInfo TCB Levels + 1
	// additional bucket is for PCK Certs that doesn't match any TCB Level
	this->buckets.resize ( this->tcbInfo.getTcbLevels ().size () + 1 );

	PCKSorter::comp_res_t res = comp_res_t::COMP_ERROR;

	// iterate all PCKs
	for ( uint32_t pck_index = 0; pck_index < this->pcks.size (); pck_index++ )
	{
		bool found = false;

		// default bucket is the last (all TCB Levels are higher than current PCK)
		size_t bucket_index = this->buckets.size () - 1;

		// read PCK TCB
		const vector<uint8_t>& pckComponents = this->pcks[pck_index]->getTcb ().getSgxTcbComponents ();
		const int64_t pck_pcesvn = this->pcks[pck_index]->getTcb ().getPceSvn ();

		// search for TCB Level (bucket) match for current PCK
		// TCB levels are ordered, first level is highest
		uint32_t level_index = 0;
		for ( auto it = this->tcbInfo.getTcbLevels ().cbegin (); it != this->tcbInfo.getTcbLevels ().cend (); ++it )
		{
			// read TCB Level Components and PCESVN, and compare to current PCK
			const vector<uint8_t>& levelComponents = (*it).getCpuSvn ();
			const int64_t level_pcesvn = (*it).getPceSvn ();
			res = this->compare_tcb_components ( pckComponents, pck_pcesvn, levelComponents, level_pcesvn );

			// PCK TCB is higher-or-equal current TCB Level, select the bucket index, stop compare
			if ( res == comp_res_t::COMP_EQUAL_OR_GREATER )
			{
				bucket_index = level_index;
				found = true;
				break;
			}
			level_index++;
		}

		// now insert PCK index to selected bucket
		bool inserted = false;

		// compare PCK to insert with current bucket PCKs, higher first
		for ( uint32_t bucket_offset = 0; bucket_offset < this->buckets[bucket_index].size (); bucket_offset++ )
		{
			uint32_t compared_pck_index = this->buckets[bucket_index][bucket_offset];
			const vector<uint8_t>& curComponents = this->pcks[compared_pck_index]->getTcb ().getSgxTcbComponents ();
			const int64_t curPcesvn = this->pcks[compared_pck_index]->getTcb ().getPceSvn ();
			res = this->compare_tcb_components ( pckComponents, pck_pcesvn, curComponents, curPcesvn );

			// PCK to insert is higher than current PCK in bucket, insert here, stop compare
			if ( res == comp_res_t::COMP_EQUAL_OR_GREATER )
			{
				this->buckets[bucket_index].insert ( this->buckets[bucket_index].begin () + bucket_offset, pck_index );
				inserted = true;
				break;
			}
		}
		// PCK to insert was not higher than any PCK in bucket, insert in the end
		if ( !inserted )
		{
			this->buckets[bucket_index].push_back ( pck_index );
		}
	}
}


/**
 * Compare raw TCB to sorted PCKs in buckets and find first match.
 * The first PCK that the raw TCB (CPUSVN and PCESVN) is equal or higher than PCK TCB is the returned PCK.
 *
 * @param [out] best_cert_index - uint32_t*, Index of best PCK in pem_certs array, can't be NULL, valid only if function returns PCK_CERT_SELECT_SUCCESS.
 * @return @ref pck_cert_selection_res_t
 */
pck_cert_selection_res_t PCKSorter::find_best_pck ( uint32_t* best_cert_index )
{
	// read raw CPUSVN into vector and decompose to TCB Components
	const vector<uint8_t> rawCPUSVN ( this->platformSvn.bytes, this->platformSvn.bytes + CPUSVN_SIZE );
	
	// only tcbInfo::V2,tcbInfo::v3 supported. TcbType field not present, default to 0
	int tcbType = 0;
	if ( this->tcbInfo.getVersion () != static_cast<unsigned int>( TcbInfo::Version::V2 ) &&
	  this->tcbInfo.getVersion () != static_cast<unsigned int>( TcbInfo::Version::V3 ) )
	{
		return PCK_CERT_SELECT_INVALID_TCB;
	}
	tcbType = this->tcbInfo.getTcbType ();
	vector < uint8_t > components;
	this->tcbmgr.decompose_cpusvn_components(rawCPUSVN, components);
	
	// iterate through ordered buckets, find first PCK with TCB lower than raw TCB
	for ( size_t bucket_index = 0; bucket_index < this->buckets.size(); bucket_index++ )
	{
		// iterate through sorted PCKs in bucket
		for ( size_t bucket_offset = 0; bucket_offset < this->buckets[bucket_index].size(); bucket_offset++ )
		{
			uint32_t cur_pck_index = this->buckets[bucket_index][bucket_offset];
			comp_res_t res = this->compare_tcb_components ( 
				components, 
				this->pceIsvSvn, 
				this->pcks[cur_pck_index]->getTcb ().getSgxTcbComponents (),
				this->pcks[cur_pck_index]->getTcb ().getPceSvn () );
			
			// found the first match PCK, return its index
			if ( res == comp_res_t::COMP_EQUAL_OR_GREATER )
			{
				*best_cert_index = cur_pck_index;
				return PCK_CERT_SELECT_SUCCESS;
			}
		}
	}

	// raw TCB lower than all PCKs, no match PCK found
	return PCK_CERT_SELECT_PCK_NOT_FOUND;
}

/**
 * parse_input_tcb function to do a jeneral check for tcb
 * just handle over the result from tcbmgr to caller
 * @param [in] void - pck_cert_selection_res_t, The result to return.
 * @return [ret] pck_cert_selection_res_t
 */
pck_cert_selection_res_t PCKSorter::parse_input_tcb(void)
{
	pck_cert_selection_res_t ret = PCK_CERT_SELECT_SUCCESS;
	ret = tcbmgr.tcb_parse_wrapper();

	// if no error, keep one copy tcbInfo in pck_sorter
	if (ret == PCK_CERT_SELECT_SUCCESS)
		tcbInfo = tcbmgr.get_tcb_info();
	return ret;
}



/*
 * Public method documented in header.
 */
pck_cert_selection_res_t PCKSorter::select_best_pck ( uint32_t* best_cert_index )
{
	// parse and validate input 
	pck_cert_selection_res_t res = this->parse_input_tcb_and_pcks ();
	if ( res != PCK_CERT_SELECT_SUCCESS )
	{
		return res;
	}

	// sort PCKs to buckets
	this->sort_to_buckets ();

	// find first match PCK
	return this->find_best_pck ( best_cert_index );
}


