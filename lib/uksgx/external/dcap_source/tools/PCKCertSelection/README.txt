PCK Cert Selection Library
---------------------------
Windows Package contains following files:
 * PCKCertSelectionLib.dll (Release, x64)
 * PCKCertSelectionLib.lib
 * pck_cert_selection.h
 * README.txt
 * SampleData folder contains 3 PEM files and 1 JSON file

Linux Package contains following files:
 * libPCKCertSelection.so (Release, x64)
 * pck_cert_selection.h
 * README.txt
 * SampleData folder contains 3 PEM files and 1 JSON file

Prerequisites:
 * OpenSSL 1.1.0 or higher crypto dynamic object (DLL on Windows, SO on Linux) must be in system PATH or in application search PATH.

API and data structures are defined in the header.
To use library on Windows:
 * Include header from source file.
 * Make sure your project arch is x64.
 * Link project to lib file.
 * Place DLL next to executable or in system PATH.

To use library on Linux:
 * Include header from source file.
 * Make sure your project arch is x64.
 * Place SO next to executable and add directory to LD_LIBRARY_PATH or in system PATH.
 
To display sample cert content include sgx extensions:
 *openssl x509 -in pckcert.pem -text -noout -certopt ext_parse


Sample code:
------------------------------------------------------------------------------------------
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
const string CERT_FILES[] = { "pck2.pem", "pck1.pem", "pck0.pem" };
const string TCB_FILE = "tcb_info.json";

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

	// read sample PCK Certs from PEM files to strings array 
	for ( size_t i = 0; i < 3; i++ )
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
	uint32_t best_index = 0;

	// call PCK Cert Selection library with sample data input
	cout << "Call with PCESVN (6), CPUSVN { 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, expecting success with index 1\n";
	pck_cert_selection_res_t res = pck_cert_select ( &plat_svn, plat_pcesvn, plat_pceid, tcb.c_str (), pcks.data (), 3, &best_index );
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
	cout << "Call with PCESVN (4), CPUSVN { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 }, expecting success with index 0\n";
	res = pck_cert_select ( &plat_svn, plat_pcesvn, plat_pceid, tcb.c_str (), pcks.data (), 3, &best_index );
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
	res = pck_cert_select ( &plat_svn, plat_pcesvn, plat_pceid, tcb.c_str (), pcks.data (), 3, &best_index );
	if ( res == PCK_CERT_SELECT_SUCCESS )
	{
		cout << "Unexpected Success index: " << res << ", exit\n";
		return 1;
	}
	else
	{
		cout << "Error returned: " << res << "\n";
	}

	cout << "Sample successfully complete\n";

	return 0;
}
------------------------------------------------------------------------------------------
