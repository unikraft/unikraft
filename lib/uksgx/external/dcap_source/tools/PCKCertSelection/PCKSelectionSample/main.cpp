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
* @file: main.h
* @description: Sample code for PCK Cert Selection library interface
*/

#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>
using namespace std;

#include "pck_cert_selection.h"

// sample data files, relative to executable location
#ifdef LINUX
const string SAMPLE_DIR = "../SampleData/";
#else
const string SAMPLE_DIR = "..\\..\\SampleData\\";
#endif
const string CERT_FILES[] = { "pck6_sample.pem", "pck5_sample.pem", "pck4_sample.pem", "pck3_sample.pem", "pck2_sample.pem", "pck1_sample.pem", "pck0_sample.pem" };
const string TCB_FILE = "tcb_info_4.json";

/**
 * read file (certificate PEM or JSON) into std::string
 */
bool read_file_to_string ( const char* file_name, string& str )
{
	if ( file_name == NULL )
	{
		return false;
	}

	ifstream file_stream ( file_name );
	if ( !file_stream.is_open () )
	{
		return false;
	}
	stringstream string_stream;
	string_stream << file_stream.rdbuf ();
	str = string_stream.str ();

	return true;
}


/**
 * demonstrate usage of PCK Cert Selection library
 */
int main ( void )
{
	cout << "PCK Cert Selection usage sample\n";

	// TCBInfo string
	string tcb;
	string path = SAMPLE_DIR + TCB_FILE;

	// read sample TCBInfo from JSON file to string
	bool read = read_file_to_string ( path.c_str (), tcb );
	if ( !read )
	{
		cout << "Failed read TCB file, exit\n";
		return 1;
	}

	// trick to keep PCKs strings alive
	vector < string > strs;
	vector < const char* > pcks;
	uint32_t cert_count = sizeof(CERT_FILES)/ sizeof(CERT_FILES[0]);

	// read sample PCK Certs from PEM files to strings array 
	for ( size_t i = 0; i < cert_count; i++ )
	{
		string pem;
		path = SAMPLE_DIR + CERT_FILES[i];
		read = read_file_to_string ( path.c_str (), pem );
		if ( !read )
		{
			cout << "Failed read PEM file, exit\n";
			return 1;
		}
		// trick to keep PCKs strings alive
		strs.push_back ( pem );
		pcks.push_back ( strs[i].c_str () );
	}

	// platform TCB raw data, CPUSVN and PCESVN
	cpu_svn_t plat_svn = { 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
	uint16_t plat_pcesvn = 6;
	uint16_t plat_pceid = 0;
	uint32_t best_index = 6;

	// call PCK Cert Selection library with sample data input
	cout << "Call with PCESVN (6), CPUSVN { 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, expecting success with index 6\n";
	pck_cert_selection_res_t res = pck_cert_select ( &plat_svn, plat_pcesvn, plat_pceid, tcb.c_str (), pcks.data (), cert_count, &best_index );
	if ( res == PCK_CERT_SELECT_SUCCESS )
	{
		cout << "Best PCK is: " << best_index << "\n";
	}
	else
	{
		cout << "Unexpected Error returned: " << res << ", exit\n";
		return 1;
	}

	// change platform TCB raw data and call PCK Cert Selection library again, sample data is same
	plat_svn = { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 };
	plat_pcesvn = 4;
	cout << "Call with PCESVN (4), CPUSVN { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 }, expecting success with index 4\n";
	res = pck_cert_select ( &plat_svn, plat_pcesvn, plat_pceid, tcb.c_str (), pcks.data (), cert_count, &best_index );
	if ( res == PCK_CERT_SELECT_SUCCESS )
	{
		cout << "Best PCK is: " << best_index << "\n";
	}
	else
	{
		cout << "Unexpected Error returned: " << res << ", exit\n";
		return 1;
	}

	// change platform TCB raw data and call PCK Cert Selection library again, sample data is same
	plat_svn = { 0 };
	plat_pcesvn = 1;
	cout << "Call with PCESVN (1), CPUSVN { 0 }, expecting fail not found (error 12)\n";
	res = pck_cert_select ( &plat_svn, plat_pcesvn, plat_pceid, tcb.c_str (), pcks.data (), cert_count, &best_index );
	if ( res == PCK_CERT_SELECT_SUCCESS )
	{
		cout << " Success index: " << res << ", exit\n";
		return 1;
	}
	else
	{
		cout << "Error returned: " << res << "\n";
	}
	// platform TCB raw data, CPUSVN and PCESVN
	plat_svn = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1 };
	plat_pcesvn = 7;

	// call PCK Cert Selection library with sample data input
	cout << "Call with PCESVN (7), CPUSVN { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1 }, expecting success with index 6\n";
	res = pck_cert_select ( &plat_svn, plat_pcesvn, plat_pceid, tcb.c_str (), pcks.data (), cert_count, &best_index );
	if ( res == PCK_CERT_SELECT_SUCCESS )
	{
		cout << "Best PCK is: " << best_index << "\n";
	}
	else
	{
		cout << "Unexpected Error returned: " << res << ", exit\n";
		return 1;
	}

	cout << "Below test case to get the hw config ID" << endl;
	
	// check the hw config 
	plat_svn = { 0, 1, 2, 3, 4, 5, 19, 7, 8, 9, 0, 0, 0, 0, 0, 0 };
	plat_pcesvn = 4;
	uint32_t configuration_id = 0;
	cout << "Call with PCESVN (4), CPUSVN {0, 1, 2, 3, 4, 5, 19, 7, 8, 9, 0, 0, 0, 0, 0, 0 }, expecting success with HW type 19\n";
	res = platform_sgx_hw_config(&plat_svn, tcb.c_str(), &configuration_id);
	if (res == PCK_CERT_SELECT_SUCCESS)
	{
		cout << "Success get hw type : " << configuration_id << endl;
		return 1;
	}
	else
	{
		cout << "Error returned: " << res << "\n";
	}

	cout << "Sample successfully complete\n";
	return 0;
}
