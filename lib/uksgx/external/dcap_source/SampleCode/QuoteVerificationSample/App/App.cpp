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


#include <stdio.h>
#include <vector>
#include <string>
#include <assert.h>
#include <fstream>
#include <sgx_uae_launch.h>
#include "sgx_urts.h"
#include "Enclave_u.h"
#include "sgx_ql_quote.h"
#include "sgx_dcap_quoteverify.h"

#ifndef _MSC_VER

#define SAMPLE_ISV_ENCLAVE  "enclave.signed.so"
#define DEFAULT_QUOTE   "../QuoteGenerationSample/quote.dat"

#else

#define SAMPLE_ISV_ENCLAVE  "enclave.signed.dll"
#define DEFAULT_QUOTE   "..\\..\\..\\QuoteGenerationSample\\x64\\Debug\\quote.dat"

#define strncpy	strncpy_s
#endif


using namespace std;


vector<uint8_t> readBinaryContent(const string& filePath)
{
    ifstream file(filePath, ios::binary);
    if (!file.is_open())
    {
        printf("Error: Unable to open quote file %s\n", filePath.c_str());
        return {};
    }

    file.seekg(0, ios_base::end);
    streampos fileSize = file.tellg();

    file.seekg(0, ios_base::beg);
    vector<uint8_t> retVal(fileSize);
    file.read(reinterpret_cast<char*>(retVal.data()), fileSize);
    file.close();
    return retVal;
}
#define PATHSIZE 0x418U


/**
 * @param quote - ECDSA quote buffer
 * @param use_qve - Set quote verification mode
 *                   If true, quote verification will be performed by Intel QvE
 *                   If false, quote verification will be performed by untrusted QVL
 */

int ecdsa_quote_verification(vector<uint8_t> quote, bool use_qve)
{
    int ret = 0;
    time_t current_time = 0;
    uint32_t supplemental_data_size = 0;
    uint8_t *p_supplemental_data = NULL;
    sgx_status_t sgx_ret = SGX_SUCCESS;
    quote3_error_t dcap_ret = SGX_QL_ERROR_UNEXPECTED;
    sgx_ql_qv_result_t quote_verification_result = SGX_QL_QV_RESULT_UNSPECIFIED;
    sgx_ql_qe_report_info_t qve_report_info;
    unsigned char rand_nonce[16] = "59jslk201fgjmm;";
    uint32_t collateral_expiration_status = 1;

    int updated = 0;
    quote3_error_t verify_qveid_ret = SGX_QL_ERROR_UNEXPECTED;
    sgx_enclave_id_t eid = 0;
    sgx_launch_token_t token = { 0 };


    // Trusted quote verification
    if (use_qve) {

        //set nonce
        //
        memcpy(qve_report_info.nonce.rand, rand_nonce, sizeof(rand_nonce));

        //get target info of SampleISVEnclave. QvE will target the generated report to this enclave.
        //
        sgx_ret = sgx_create_enclave(SAMPLE_ISV_ENCLAVE, SGX_DEBUG_FLAG, &token, &updated, &eid, NULL);
        if (sgx_ret != SGX_SUCCESS) {
            printf("\tError: Can't load SampleISVEnclave. 0x%04x\n", sgx_ret);
            return -1;
        }
        sgx_status_t get_target_info_ret;
        sgx_ret = ecall_get_target_info(eid, &get_target_info_ret, &qve_report_info.app_enclave_target_info);
        if (sgx_ret != SGX_SUCCESS || get_target_info_ret != SGX_SUCCESS) {
            printf("\tError in sgx_get_target_info. 0x%04x\n", get_target_info_ret);
        }
        else {
            printf("\tInfo: get target info successfully returned.\n");
        }

        //call DCAP quote verify library to set QvE loading policy
        //
        dcap_ret = sgx_qv_set_enclave_load_policy(SGX_QL_DEFAULT);
        if (dcap_ret == SGX_QL_SUCCESS) {
            printf("\tInfo: sgx_qv_set_enclave_load_policy successfully returned.\n");
        }
        else {
            printf("\tError: sgx_qv_set_enclave_load_policy failed: 0x%04x\n", dcap_ret);
        }

        //call DCAP quote verify library to get supplemental data size
        //
        dcap_ret = sgx_qv_get_quote_supplemental_data_size(&supplemental_data_size);
        if (dcap_ret == SGX_QL_SUCCESS && supplemental_data_size == sizeof(sgx_ql_qv_supplemental_t)) {
            printf("\tInfo: sgx_qv_get_quote_supplemental_data_size successfully returned.\n");
            p_supplemental_data = (uint8_t*)malloc(supplemental_data_size);
            if (p_supplemental_data != NULL) {
                memset(p_supplemental_data, 0, sizeof(supplemental_data_size));
            }
            //Just print error in sample
            //
            else {
                printf("\tError: Cannot allocate memory for supplemental data.\n");
            }
        }
        else {
            if (dcap_ret != SGX_QL_SUCCESS)
                printf("\tError: sgx_qv_get_quote_supplemental_data_size failed: 0x%04x\n", dcap_ret);

            if (supplemental_data_size != sizeof(sgx_ql_qv_supplemental_t))
                printf("\tWarning: Quote supplemental data size is different between DCAP QVL and QvE, please make sure you installed DCAP QVL and QvE from same release.\n");

            supplemental_data_size = 0;
        }

        //set current time. This is only for sample use, please use trusted time in product.
        //
        current_time = time(NULL);


        //call DCAP quote verify library for quote verification
        //here you can choose 'trusted' or 'untrusted' quote verification by specifying parameter '&qve_report_info'
        //if '&qve_report_info' is NOT NULL, this API will call Intel QvE to verify quote
        //if '&qve_report_info' is NULL, this API will call 'untrusted quote verify lib' to verify quote, this mode doesn't rely on SGX capable system, but the results can not be cryptographically authenticated
        dcap_ret = sgx_qv_verify_quote(
            quote.data(), (uint32_t)quote.size(),
            NULL,
            current_time,
            &collateral_expiration_status,
            &quote_verification_result,
            &qve_report_info,
            supplemental_data_size,
            p_supplemental_data);
        if (dcap_ret == SGX_QL_SUCCESS) {
            printf("\tInfo: App: sgx_qv_verify_quote successfully returned.\n");
        }
        else {
            printf("\tError: App: sgx_qv_verify_quote failed: 0x%04x\n", dcap_ret);
        }


        // Threshold of QvE ISV SVN. The ISV SVN of QvE used to verify quote must be greater or equal to this threshold
        // e.g. You can check latest QvE ISVSVN from QvE configuration file on Github
        // https://github.com/intel/SGXDataCenterAttestationPrimitives/blob/master/QuoteVerification/QvE/Enclave/linux/config.xml#L4
        // or you can get latest QvE ISVSVN in QvE Identity JSON file from
        // https://api.trustedservices.intel.com/sgx/certification/v3/qve/identity
        // Make sure you are using trusted & latest QvE ISV SVN as threshold
        // Warning: The function may return erroneous result if QvE ISV SVN has been modified maliciously.
        //
        sgx_isv_svn_t qve_isvsvn_threshold = 6;

        //call sgx_dcap_tvl API in SampleISVEnclave to verify QvE's report and identity
        //
        sgx_ret = sgx_tvl_verify_qve_report_and_identity(eid,
            &verify_qveid_ret,
            quote.data(),
            (uint32_t) quote.size(),
            &qve_report_info,
            current_time,
            collateral_expiration_status,
            quote_verification_result,
            p_supplemental_data,
            supplemental_data_size,
            qve_isvsvn_threshold);

        if (sgx_ret != SGX_SUCCESS || verify_qveid_ret != SGX_QL_SUCCESS) {
            printf("\tError: Ecall: Verify QvE report and identity failed. 0x%04x\n", verify_qveid_ret);
        }
        else {
            printf("\tInfo: Ecall: Verify QvE report and identity successfully returned.\n");
        }

        //check verification result
        //
        switch (quote_verification_result)
        {
        case SGX_QL_QV_RESULT_OK:
            //check verification collateral expiration status
            //this value should be considered in your own attestation/verification policy
            //
            if (collateral_expiration_status == 0) {
                printf("\tInfo: App: Verification completed successfully.\n");
                ret = 0;
            }
            else {
                printf("\tWarning: App: Verification completed, but collateral is out of date based on 'expiration_check_date' you provided.\n");
                ret = 1;
            }

            break;
        case SGX_QL_QV_RESULT_CONFIG_NEEDED:
        case SGX_QL_QV_RESULT_OUT_OF_DATE:
        case SGX_QL_QV_RESULT_OUT_OF_DATE_CONFIG_NEEDED:
        case SGX_QL_QV_RESULT_SW_HARDENING_NEEDED:
        case SGX_QL_QV_RESULT_CONFIG_AND_SW_HARDENING_NEEDED:
            printf("\tWarning: App: Verification completed with Non-terminal result: %x\n", quote_verification_result);
            ret = 1;
            break;
        case SGX_QL_QV_RESULT_INVALID_SIGNATURE:
        case SGX_QL_QV_RESULT_REVOKED:
        case SGX_QL_QV_RESULT_UNSPECIFIED:
        default:
            printf("\tError: App: Verification completed with Terminal result: %x\n", quote_verification_result);
            ret = -1;
            break;
        }

        //check supplemental data if necessary
        //
        if (p_supplemental_data != NULL && supplemental_data_size > 0) {
            sgx_ql_qv_supplemental_t *p = (sgx_ql_qv_supplemental_t*)p_supplemental_data;

            //you can check supplemental data based on your own attestation/verification policy
            //here we only print supplemental data version for demo usage
            //
            printf("\tInfo: Supplemental data version: %d\n", p->version);
        }

    }



    // Untrusted quote verification
    else {
        //call DCAP quote verify library to get supplemental data size
        //
        dcap_ret = sgx_qv_get_quote_supplemental_data_size(&supplemental_data_size);
        if (dcap_ret == SGX_QL_SUCCESS && supplemental_data_size == sizeof(sgx_ql_qv_supplemental_t)) {
            printf("\tInfo: sgx_qv_get_quote_supplemental_data_size successfully returned.\n");
            p_supplemental_data = (uint8_t*)malloc(supplemental_data_size);
            if (p_supplemental_data != NULL) {
                memset(p_supplemental_data, 0, sizeof(supplemental_data_size));
            }
            //Just print error in sample
            //
            else {
                printf("\tError: Cannot allocate memory for supplemental data.\n");
            }
        }
        else {
            if (dcap_ret != SGX_QL_SUCCESS)
                printf("\tError: sgx_qv_get_quote_supplemental_data_size failed: 0x%04x\n", dcap_ret);

            if (supplemental_data_size != sizeof(sgx_ql_qv_supplemental_t))
                printf("\tWarning: Quote supplemental data size is different between DCAP QVL and QvE, please make sure you installed DCAP QVL and QvE from same release.\n");

            supplemental_data_size = 0;
        }

        //set current time. This is only for sample purposes, in production mode a trusted time should be used.
        //
        current_time = time(NULL);


        //call DCAP quote verify library for quote verification
        //here you can choose 'trusted' or 'untrusted' quote verification by specifying parameter '&qve_report_info'
        //if '&qve_report_info' is NOT NULL, this API will call Intel QvE to verify quote
        //if '&qve_report_info' is NULL, this API will call 'untrusted quote verify lib' to verify quote, this mode doesn't rely on SGX capable system, but the results can not be cryptographically authenticated
        dcap_ret = sgx_qv_verify_quote(
            quote.data(), (uint32_t)quote.size(),
            NULL,
            current_time,
            &collateral_expiration_status,
            &quote_verification_result,
            NULL,
            supplemental_data_size,
            p_supplemental_data);
        if (dcap_ret == SGX_QL_SUCCESS) {
            printf("\tInfo: App: sgx_qv_verify_quote successfully returned.\n");
        }
        else {
            printf("\tError: App: sgx_qv_verify_quote failed: 0x%04x\n", dcap_ret);
        }

        //check verification result
        //
        switch (quote_verification_result)
        {
        case SGX_QL_QV_RESULT_OK:
            //check verification collateral expiration status
            //this value should be considered in your own attestation/verification policy
            //
            if (collateral_expiration_status == 0) {
                printf("\tInfo: App: Verification completed successfully.\n");
                ret = 0;
            }
            else {
                printf("\tWarning: App: Verification completed, but collateral is out of date based on 'expiration_check_date' you provided.\n");
                ret = 1;
            }
            break;
        case SGX_QL_QV_RESULT_CONFIG_NEEDED:
        case SGX_QL_QV_RESULT_OUT_OF_DATE:
        case SGX_QL_QV_RESULT_OUT_OF_DATE_CONFIG_NEEDED:
        case SGX_QL_QV_RESULT_SW_HARDENING_NEEDED:
        case SGX_QL_QV_RESULT_CONFIG_AND_SW_HARDENING_NEEDED:
            printf("\tWarning: App: Verification completed with Non-terminal result: %x\n", quote_verification_result);
            ret = 1;
            break;
        case SGX_QL_QV_RESULT_INVALID_SIGNATURE:
        case SGX_QL_QV_RESULT_REVOKED:
        case SGX_QL_QV_RESULT_UNSPECIFIED:
        default:
            printf("\tError: App: Verification completed with Terminal result: %x\n", quote_verification_result);
            ret = -1;
            break;
        }

        //check supplemental data if necessary
        //
        if (p_supplemental_data != NULL && supplemental_data_size > 0) {
            sgx_ql_qv_supplemental_t *p = (sgx_ql_qv_supplemental_t*)p_supplemental_data;

            //you can check supplemental data based on your own attestation/verification policy
            //here we only print supplemental data version for demo usage
            //
            printf("\tInfo: Supplemental data version: %d\n", p->version);
        }

    }


    if (p_supplemental_data) {
        free(p_supplemental_data);
    }

    if (eid) {
        sgx_destroy_enclave(eid);
    }

    return ret;
}

void usage()
{
    printf("\nUsage:\n");
    printf("\tPlease specify quote path, e.g. \"./app -quote <path/to/quote>\"\n");
    printf("\tDefault quote path is %s when no command line args\n\n", DEFAULT_QUOTE);
}


/* Application entry */
int SGX_CDECL main(int argc, char *argv[])
{
    vector<uint8_t> quote;

    char quote_path[PATHSIZE] = { '\0' };

    //Just for sample use, better to change solid command line args solution in production env
    if (argc != 1 && argc != 3) {
        usage();
        return 0;
    }

    if (argv[1] && argv[2]) {
        if (!strcmp(argv[1], "-quote")) {
            strncpy(quote_path, argv[2], PATHSIZE - 1);
        }
    }

    if (*quote_path == '\0') {
        strncpy(quote_path, DEFAULT_QUOTE, PATHSIZE - 1);
    }

    //read quote from file
    //
    quote = readBinaryContent(quote_path);
    if (quote.empty()) {
        usage();
        return -1;
    }

    printf("Info: ECDSA quote path: %s\n", quote_path);


    //We demonstrate two different types of quote verification
    //   a. Trusted quote verification - quote will be verified by Intel QvE
    //   b. Untrusted quote verification - quote will be verified by untrusted QVL (Quote Verification Library)
    //      this mode doesn't rely on SGX capable system, but the results can not be cryptographically authenticated
    //

    // Trusted quote verification, ignore error checking
    printf("\nTrusted quote verification:\n");
    ecdsa_quote_verification(quote, true);

    printf("\n===========================================\n");

    // Unrusted quote verification, ignore error checking
    printf("\nUntrusted quote verification:\n");
    ecdsa_quote_verification(quote, false);

    printf("\n");

    return 0;
}
