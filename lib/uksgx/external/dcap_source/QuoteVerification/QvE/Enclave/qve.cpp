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

 /*
 * Quote Verification Enclave (QvE)
 * An architectural enclave for quote verification.
 */

#ifndef SGX_TRUSTED
#define get_fmspc_ca_from_quote qvl_get_fmspc_ca_from_quote
#define sgx_qve_verify_quote sgx_qvl_verify_quote
#define sgx_qve_get_quote_supplemental_data_size sgx_qvl_get_quote_supplemental_data_size
#define sgx_qve_get_quote_supplemental_data_version sgx_qvl_get_quote_supplemental_data_version
#include "sgx_dcap_qv_internal.h"
#define memset_s(a,b,c,d) memset(a,c,d)
#define memcpy_s(a,b,c,d) (memcpy(a,c,b) && 0)
#define sgx_is_within_enclave(a,b) (1)
#else //SGX_TRUSTED
#include "qve_t.h"
#include <mbusafecrt.h>
#include <sgx_tcrypto.h>
#include <sgx_trts.h>
#include <sgx_utils.h>
#endif //SGX_TRUSTED

#define __STDC_WANT_LIB_EXT1__ 1
#include <string>
#include <cstring>
#include <array>
#include <algorithm>
#include <vector>
#include "Verifiers/EnclaveIdentityParser.h"
#include "Verifiers/EnclaveIdentityV2.h"
#include "QuoteVerification/Quote.h"
#include "PckParser/CrlStore.h"
#include "CertVerification/CertificateChain.h"
#include "Utils/TimeUtils.h"
#include "SgxEcdsaAttestation/AttestationParsers.h"
#include "sgx_qve_header.h"
#include "sgx_qve_def.h"


using namespace intel::sgx::dcap;
using namespace intel::sgx::dcap::parser;

//Intel Root Public Key
//
const uint8_t INTEL_ROOT_PUB_KEY[] = {
    0x04, 0x0b, 0xa9, 0xc4, 0xc0, 0xc0, 0xc8, 0x61, 0x93, 0xa3, 0xfe, 0x23, 0xd6, 0xb0, 0x2c,
    0xda, 0x10, 0xa8, 0xbb, 0xd4, 0xe8, 0x8e, 0x48, 0xb4, 0x45, 0x85, 0x61, 0xa3, 0x6e, 0x70,
    0x55, 0x25, 0xf5, 0x67, 0x91, 0x8e, 0x2e, 0xdc, 0x88, 0xe4, 0x0d, 0x86, 0x0b, 0xd0, 0xcc,
    0x4e, 0xe2, 0x6a, 0xac, 0xc9, 0x88, 0xe5, 0x05, 0xa9, 0x53, 0x55, 0x8c, 0x45, 0x3f, 0x6b,
    0x09, 0x04, 0xae, 0x73, 0x94
};

/**
 * Check if a given status code is an expiration error or not.
 *
 * @param status_err[IN] - Status error code.
 *
 * @return 1: Status indicates an expiration error.
 * @return 0: Status indicates error other than expiration error.
*
 **/
static bool is_nonterminal_error(Status status_err) {
    switch (status_err)
    {
    case STATUS_TCB_OUT_OF_DATE:
    case STATUS_TCB_OUT_OF_DATE_CONFIGURATION_NEEDED:
    case STATUS_SGX_ENCLAVE_IDENTITY_OUT_OF_DATE:
    case STATUS_SGX_ENCLAVE_REPORT_ISVSVN_OUT_OF_DATE:
    case STATUS_QE_IDENTITY_OUT_OF_DATE:
    case STATUS_SGX_TCB_INFO_EXPIRED:
    case STATUS_SGX_PCK_CERT_CHAIN_EXPIRED:
    case STATUS_SGX_CRL_EXPIRED:
    case STATUS_SGX_SIGNING_CERT_CHAIN_EXPIRED:
    case STATUS_SGX_ENCLAVE_IDENTITY_EXPIRED:
    case STATUS_TCB_CONFIGURATION_NEEDED:
    case STATUS_TCB_SW_HARDENING_NEEDED:
    case STATUS_TCB_CONFIGURATION_AND_SW_HARDENING_NEEDED:
        return true;
    default:
        return false;
    }
}

/**
 * Check if a given status code is an expiration error or not.
 *
 * @param status_err[IN] - Status error code.
 *
 * @return 1: Status indicates an expiration error.
 * @return 0: Status indicates error other than expiration error.
*
 **/
static bool is_expiration_error(Status status_err) {
    switch (status_err)
    {
    case STATUS_SGX_TCB_INFO_EXPIRED:
    case STATUS_SGX_PCK_CERT_CHAIN_EXPIRED:
    case STATUS_SGX_CRL_EXPIRED:
    case STATUS_SGX_SIGNING_CERT_CHAIN_EXPIRED:
    case STATUS_SGX_ENCLAVE_IDENTITY_EXPIRED:
        return true;
    default:
        return false;
    }
}

/**
 * Map Status error code to quote3_error_t error code.
 *
 * @param status_err[IN] - Status error code.
 *
 * @return quote3_error_t that matches status_err.
*
 **/
static quote3_error_t status_error_to_quote3_error(Status status_err) {

    switch (status_err)
    {
    case STATUS_OK:
        return SGX_QL_SUCCESS;
    case STATUS_MISSING_PARAMETERS:
        return SGX_QL_ERROR_INVALID_PARAMETER;
    case STATUS_UNSUPPORTED_QUOTE_FORMAT:
        return SGX_QL_QUOTE_FORMAT_UNSUPPORTED;
    case STATUS_INVALID_QE_REPORT_SIGNATURE:
        return SGX_QL_QE_REPORT_INVALID_SIGNATURE;
    case STATUS_SGX_ENCLAVE_REPORT_UNSUPPORTED_FORMAT:
        return SGX_QL_QE_REPORT_UNSUPPORTED_FORMAT;
    case STATUS_UNSUPPORTED_PCK_CERT_FORMAT:
    case STATUS_UNSUPPORTED_CERT_FORMAT:
        return SGX_QL_PCK_CERT_UNSUPPORTED_FORMAT;
    case STATUS_INVALID_PCK_CERT:
    case STATUS_SGX_PCK_CERT_CHAIN_UNTRUSTED:
    case STATUS_SGX_ROOT_CA_UNTRUSTED:
        return SGX_QL_PCK_CERT_CHAIN_ERROR;
    case STATUS_UNSUPPORTED_TCB_INFO_FORMAT:
    case STATUS_TCB_NOT_SUPPORTED:
    case STATUS_SGX_TCB_INFO_UNSUPPORTED_FORMAT:
    case STATUS_SGX_TCB_INFO_INVALID:
        return SGX_QL_TCBINFO_UNSUPPORTED_FORMAT;
    case STATUS_TCB_INFO_MISMATCH:
        return SGX_QL_TCBINFO_MISMATCH;
    case STATUS_SGX_QE_IDENTITY_UNSUPPORTED_FORMAT:
        return SGX_QL_QEIDENTITY_UNSUPPORTED_FORMAT;
    case STATUS_SGX_QE_IDENTITY_INVALID:
    case STATUS_QE_IDENTITY_MISMATCH:
    case STATUS_SGX_ENCLAVE_REPORT_MISCSELECT_MISMATCH:
    case STATUS_SGX_ENCLAVE_REPORT_ATTRIBUTES_MISMATCH:
    case STATUS_SGX_ENCLAVE_REPORT_MRENCLAVE_MISMATCH:
    case STATUS_SGX_ENCLAVE_REPORT_MRSIGNER_MISMATCH:
    case STATUS_SGX_ENCLAVE_REPORT_ISVPRODID_MISMATCH:
        return SGX_QL_QEIDENTITY_MISMATCH;
    case STATUS_TCB_OUT_OF_DATE:
        return SGX_QL_TCB_OUT_OF_DATE;
    case STATUS_TCB_CONFIGURATION_NEEDED:
        return SGX_QL_TCB_CONFIGURATION_NEEDED;
    case STATUS_TCB_OUT_OF_DATE_CONFIGURATION_NEEDED:
        return SGX_QL_TCB_OUT_OF_DATE_CONFIGURATION_NEEDED;
    case STATUS_SGX_ENCLAVE_IDENTITY_OUT_OF_DATE:
        return SGX_QL_SGX_ENCLAVE_IDENTITY_OUT_OF_DATE;
    case STATUS_SGX_ENCLAVE_REPORT_ISVSVN_OUT_OF_DATE:
        return SGX_QL_SGX_ENCLAVE_REPORT_ISVSVN_OUT_OF_DATE;
    case STATUS_QE_IDENTITY_OUT_OF_DATE:
        return SGX_QL_QE_IDENTITY_OUT_OF_DATE;
    case STATUS_SGX_TCB_INFO_EXPIRED:
        return SGX_QL_SGX_TCB_INFO_EXPIRED;
    case STATUS_SGX_PCK_CERT_CHAIN_EXPIRED:
        return SGX_QL_SGX_PCK_CERT_CHAIN_EXPIRED;
    case STATUS_SGX_CRL_EXPIRED:
        return SGX_QL_SGX_CRL_EXPIRED;
    case STATUS_SGX_SIGNING_CERT_CHAIN_EXPIRED:
        return SGX_QL_SGX_SIGNING_CERT_CHAIN_EXPIRED;
    case STATUS_SGX_ENCLAVE_IDENTITY_EXPIRED:
        return SGX_QL_SGX_ENCLAVE_IDENTITY_EXPIRED;
    case STATUS_PCK_REVOKED:
    case STATUS_SGX_PCK_REVOKED:
    case STATUS_SGX_INTERMEDIATE_CA_REVOKED:
    case STATUS_SGX_TCB_SIGNING_CERT_REVOKED:
        return SGX_QL_PCK_REVOKED;
    case STATUS_TCB_REVOKED:
        return SGX_QL_TCB_REVOKED;
    case STATUS_UNSUPPORTED_QE_CERTIFICATION:
    case STATUS_UNSUPPORTED_QE_CERTIFICATION_DATA_TYPE:
        return SGX_QL_QUOTE_CERTIFICATION_DATA_UNSUPPORTED;
    case STATUS_SGX_ENCLAVE_IDENTITY_UNSUPPORTED_FORMAT:
    case STATUS_SGX_ENCLAVE_IDENTITY_INVALID:
    case STATUS_SGX_ENCLAVE_IDENTITY_UNSUPPORTED_VERSION:
    case STATUS_UNSUPPORTED_QE_IDENTITY_FORMAT:
    case STATUS_SGX_ENCLAVE_IDENTITY_INVALID_SIGNATURE:
        return SGX_QL_QEIDENTITY_CHAIN_ERROR;
    case STATUS_TCB_INFO_INVALID_SIGNATURE:
    case STATUS_SGX_TCB_SIGNING_CERT_CHAIN_UNTRUSTED:
        return SGX_QL_TCBINFO_CHAIN_ERROR;
    case STATUS_TCB_SW_HARDENING_NEEDED:
        return SGX_QL_TCB_SW_HARDENING_NEEDED;
    case STATUS_TCB_CONFIGURATION_AND_SW_HARDENING_NEEDED:
        return SGX_QL_TCB_CONFIGURATION_AND_SW_HARDENING_NEEDED;
    case STATUS_TDX_MODULE_MISMATCH:
        return SGX_QL_TDX_MODULE_MISMATCH;
    default:
        return SGX_QL_ERROR_UNEXPECTED;
    }
}

/**
 * Map Status error code to sgx_ql_qv_result_t.
 *
 * @param status_err[IN] - Status error code.
 *
 * @return sgx_ql_qv_result_t that matches status_err.
 *
 **/
static sgx_ql_qv_result_t status_error_to_ql_qve_result(Status status_err) {
    switch (status_err)
    {
    case STATUS_OK:
        return SGX_QL_QV_RESULT_OK;
    case STATUS_SGX_ENCLAVE_IDENTITY_INVALID_SIGNATURE:
    case STATUS_SGX_QE_IDENTITY_INVALID_SIGNATURE:
    case STATUS_INVALID_QUOTE_SIGNATURE:
    case STATUS_INVALID_QE_REPORT_SIGNATURE:
    case STATUS_SGX_CRL_INVALID_SIGNATURE:
    case STATUS_TCB_INFO_INVALID_SIGNATURE:
        return SGX_QL_QV_RESULT_INVALID_SIGNATURE;
    case STATUS_PCK_REVOKED:
    case STATUS_TCB_REVOKED:
    case STATUS_SGX_TCB_SIGNING_CERT_REVOKED:
    case STATUS_SGX_PCK_REVOKED:
    case STATUS_SGX_INTERMEDIATE_CA_REVOKED:
        return SGX_QL_QV_RESULT_REVOKED;
    case STATUS_TCB_CONFIGURATION_NEEDED:
        return SGX_QL_QV_RESULT_CONFIG_NEEDED;
    case STATUS_TCB_OUT_OF_DATE_CONFIGURATION_NEEDED:
        return SGX_QL_QV_RESULT_OUT_OF_DATE_CONFIG_NEEDED;
    case STATUS_TCB_OUT_OF_DATE:
    case STATUS_SGX_ENCLAVE_IDENTITY_OUT_OF_DATE:
    case STATUS_SGX_ENCLAVE_REPORT_ISVSVN_OUT_OF_DATE:
    case STATUS_QE_IDENTITY_OUT_OF_DATE:
        return SGX_QL_QV_RESULT_OUT_OF_DATE;
    case STATUS_TCB_SW_HARDENING_NEEDED:
        return SGX_QL_QV_RESULT_SW_HARDENING_NEEDED;
    case STATUS_TCB_CONFIGURATION_AND_SW_HARDENING_NEEDED:
        return SGX_QL_QV_RESULT_CONFIG_AND_SW_HARDENING_NEEDED;
    default:
        return SGX_QL_QV_RESULT_UNSPECIFIED;
    }
}

/**
 * Check the CRL is PEM encoding or not
 *
 **/
static bool check_pem_crl(char *crl, uint32_t size)
{
    if (crl == NULL || size < CRL_MIN_SIZE)
        return false;

    if (strncmp(crl, PEM_CRL_PREFIX, PEM_CRL_PREFIX_SIZE) == 0)
        return true;

    return false;
}

/**
 * Check the CRL is hex string or not
 *
 **/
static bool check_hex_crl(char *crl, uint32_t size)
{
    if (crl == NULL || size < CRL_MIN_SIZE)
        return false;

    //only check length = size-1, as the last item may be nul terminator
    for (uint32_t i = 0; i < size - 1; i++) {
        if (!isxdigit(crl[i])) {
            return false;
        }
    }

    return true;
}

/**
 * Convert char to hex string
 *
 **/
static std::string bin2hex(char *in, uint32_t size)
{
    std::string result;

    if (in == NULL || size == 0)
        return result;

    const std::vector<uint8_t> crl(in, std::next(in, size));

    result.reserve(crl.size() * 2);

    static constexpr char hex[] = "0123456789ABCDEF";

    for (const uint8_t c : crl)
    {
        result.push_back(hex[c / 16]);
        result.push_back(hex[c % 16]);
    }

    return result;
}

#define SGX_TCB_LEVEL_LOWER false
#define SGX_TCB_LEVEL_EQUAL_OR_HIGHER true
#define TCB_LEVELS_COUNT 16
static bool isPCKCertSGXTCBLevelHigherOrEqual(const x509::PckCertificate& pckCert,
    const json::TcbLevel& tcbLevel)
{
    for (unsigned int index = 0; index < TCB_LEVELS_COUNT; ++index)
    {
        const auto componentValue = pckCert.getTcb().getSgxTcbComponentSvn(index);
        const auto otherComponentValue = tcbLevel.getSgxTcbComponentSvn(index);
        if (componentValue < otherComponentValue)
        {
            // If *ANY* SGX_TCB_LEVEL component is lower then PCKCertSGXTCBLevel is considered lower
            return SGX_TCB_LEVEL_LOWER;
        }
    }
    return SGX_TCB_LEVEL_EQUAL_OR_HIGHER;
}

static time_t getMatchingTcbLevelTcbDate(const std::set<json::TcbLevel, std::greater<json::TcbLevel>> &tcbs,
    const x509::PckCertificate &pckCert)
{
    const auto certPceSvn = pckCert.getTcb().getPceSvn();

    for (const auto& tcb : tcbs)
    {
        if (isPCKCertSGXTCBLevelHigherOrEqual(pckCert, tcb) && certPceSvn >= tcb.getPceSvn())
        {
            return tcb.getTcbDate();
        }
    }
    return 0;
}




/**
 * Given a quote with cert type 5, extract PCK Cert chain and return it.
 * @param p_quote[IN] - Pointer to a quote buffer.
 * @param quote_size[IN] - Size of input quote buffer.
 * @param p_pck_cert_chain_size[OUT] - Pointer to a extracted chain size.
 * @param pp_pck_cert_chain[OUT] - Pointer to a pointer to a buffer to write PCK Cert chain to.
 *
 * @return quote3_error_t code of the operation, one of:
 *      - SGX_QL_SUCCESS
 *      - SGX_QL_ERROR_INVALID_PARAMETER
 *      - SGX_QL_PCK_CERT_UNSUPPORTED_FORMAT
 *      - SGX_QL_QUOTE_CERTIFICATION_DATA_UNSUPPORTED
 *      - SGX_QL_ERROR_UNEXPECTED
 **/
static quote3_error_t extract_chain_from_quote(const uint8_t *p_quote,
    uint32_t quote_size,
    uint32_t* p_pck_cert_chain_size,
    uint8_t** pp_pck_cert_chain) {

    if (p_quote == NULL || quote_size < QUOTE_MIN_SIZE || p_pck_cert_chain_size == NULL || pp_pck_cert_chain == NULL || *pp_pck_cert_chain != NULL) {
        return SGX_QL_ERROR_INVALID_PARAMETER;
    }

    Status pck_res = STATUS_MISSING_PARAMETERS;
    quote3_error_t ret = SGX_QL_ERROR_UNEXPECTED;
    uint16_t p_pck_cert_chain_type = 0;
    do {
        //get certification data size
        //
        pck_res = sgxAttestationGetQECertificationDataSize(
            p_quote,
            quote_size,
            p_pck_cert_chain_size);
        if (pck_res != STATUS_OK) {
            ret = status_error_to_quote3_error(pck_res);
            break;
        }
        //for some reason sgxAttestationGetQECertificationDataSize successfully returned, with chain_size == 0
        //
        if (*p_pck_cert_chain_size == 0) {
            break;
        }

        //verify quote format, allocate memory for certification data, then fill it with the certification data from the quote
        //sgxAttestationGetQECertificationDataSize doesn't calculate the value with '\0',
        //hence we need to allocate p_pck_cert_chain_size + 1 (+1 for '\0')
        //
        *pp_pck_cert_chain = (uint8_t*)malloc(1 + *p_pck_cert_chain_size);
        if (*pp_pck_cert_chain == NULL) {
            ret = SGX_QL_ERROR_OUT_OF_MEMORY;
            break;
        }

        //sgxAttestationGetQECertificationData expects p_pck_cert_chain_size to be exactly as returned
        //from sgxAttestationGetQECertificationDataSize
        //
        pck_res = sgxAttestationGetQECertificationData(
            p_quote,
            quote_size,
            *p_pck_cert_chain_size,
            *pp_pck_cert_chain,
            &p_pck_cert_chain_type);
        if (pck_res != STATUS_OK) {
            ret = status_error_to_quote3_error(pck_res);
            break;
        }
        (*pp_pck_cert_chain)[(*p_pck_cert_chain_size)] = '\0';

        //validate quote certification type
        //
        if (p_pck_cert_chain_type != QUOTE_CERT_TYPE) {
            ret = SGX_QL_QUOTE_CERTIFICATION_DATA_UNSUPPORTED;
            break;
        }

        ret = SGX_QL_SUCCESS;
    } while (0);

    if (ret != SGX_QL_SUCCESS) {
        if (*pp_pck_cert_chain != NULL) {
            free(*pp_pck_cert_chain);
            *pp_pck_cert_chain = NULL;
        }
    }
    return ret;
}

/**
 * Extract FMSPc and CA from a given quote with cert type 5.
 * @param p_quote[IN] - Pointer to a quote buffer.
 * @param quote_size[IN] - Size of input quote buffer.
 * @param p_fmsp_from_quote[OUT] - Pointer to a buffer to write fmsp to.
 * @param fmsp_from_quote_size[IN] - Size of fmsp buffer.
 * @param p_ca_from_quote[OUT] - Pointer to a buffer to write CA to.
 * @param ca_from_quote_size[IN] - Size of CA buffer.
 *
 * @return Status code of the operation, one of:
 *      - SGX_QL_SUCCESS
 *      - SGX_QL_ERROR_INVALID_PARAMETER
 *      - SGX_QL_ATT_KEY_CERT_DATA_INVALID
 *      - SGX_QL_QUOTE_CERTIFICATION_DATA_UNSUPPORTED
 *      - SGX_QL_ERROR_UNEXPECTED
 **/
quote3_error_t get_fmspc_ca_from_quote(const uint8_t* p_quote, uint32_t quote_size,
    unsigned char* p_fmsp_from_quote, uint32_t fmsp_from_quote_size,
    unsigned char* p_ca_from_quote, uint32_t ca_from_quote_size) {
    if (p_quote == NULL ||
        quote_size < QUOTE_MIN_SIZE ||
        !sgx_is_within_enclave(p_quote, quote_size) ||
        p_fmsp_from_quote == NULL || fmsp_from_quote_size < FMSPC_SIZE ||
        !sgx_is_within_enclave(p_fmsp_from_quote, fmsp_from_quote_size) ||
        p_ca_from_quote == NULL || ca_from_quote_size < CA_SIZE ||
        !sgx_is_within_enclave(p_ca_from_quote, ca_from_quote_size))
    {
        return SGX_QL_ERROR_INVALID_PARAMETER;
    }

    quote3_error_t ret = SGX_QL_ERROR_UNEXPECTED;
    uint32_t pck_cert_chain_size = 0;
    uint8_t* p_pck_cert_chain = NULL;

    do {
        ret = extract_chain_from_quote(p_quote, quote_size, &pck_cert_chain_size, &p_pck_cert_chain);
        if (ret != SGX_QL_SUCCESS || p_pck_cert_chain == NULL || pck_cert_chain_size == 0) {
            break;
        }

        //convert certification data to string
        //
        CertificateChain chain;
        if (chain.parse((reinterpret_cast<const char*>(p_pck_cert_chain))) != STATUS_OK || chain.length() != EXPECTED_CERTIFICATE_COUNT_IN_PCK_CHAIN) {
            ret = SGX_QL_PCK_CERT_UNSUPPORTED_FORMAT;
            break;
        }
        //extract data from certificates
        //
        auto topmost_cert = chain.getTopmostCert();
        x509::PckCertificate topmost_pck_cert;
        //compare to nullptr (C++ way of comparing pointers to NULL), otherwise gcc will report a compilation warning
        //
        if (topmost_cert == nullptr) {
            ret = SGX_QL_PCK_CERT_CHAIN_ERROR;
            break;
        }
        try {
            topmost_pck_cert = x509::PckCertificate(*topmost_cert);
        }
        catch (...) {
            ret = SGX_QL_PCK_CERT_UNSUPPORTED_FORMAT;
            break;
        }

        auto fmspc_from_cert = topmost_pck_cert.getFmspc();
        auto issuer = topmost_cert->getIssuer().getCommonName();
        if (issuer.find(PROCESSOR_ISSUER) != std::string::npos) {
            if (memcpy_s(p_ca_from_quote, ca_from_quote_size, PROCESSOR_ISSUER_ID, sizeof(PROCESSOR_ISSUER_ID)) != 0) {
                ret = SGX_QL_ERROR_UNEXPECTED;
                break;
            }
        }
        else if (issuer.find(PLATFORM_ISSUER) != std::string::npos) {
            if (memcpy_s(p_ca_from_quote, ca_from_quote_size, PLATFORM_ISSUER_ID, sizeof(PLATFORM_ISSUER_ID)) != 0) {
                ret = SGX_QL_ERROR_UNEXPECTED;
                break;
            }
        }
        else {
            ret = SGX_QL_PCK_CERT_UNSUPPORTED_FORMAT;
            break;
        }
        if (memcpy_s(p_fmsp_from_quote, fmsp_from_quote_size,
            fmspc_from_cert.data(), fmspc_from_cert.size()) != 0) {
            ret = SGX_QL_ERROR_UNEXPECTED;
            break;
        }

        ret = SGX_QL_SUCCESS;
    } while (0);

    //free allocated memory
    //
    if (p_pck_cert_chain != NULL) {
        free(p_pck_cert_chain);
    }

    return ret;
}

static time_t getEarliestIssueDate(const CertificateChain* chain) {
    time_t min_issue_date = 0;
    auto certs = chain->getCerts();
    if (!certs.empty()) {
        min_issue_date = certs.front()->getValidity().getNotBeforeTime();
        for (auto const& cert : certs) {
            if (cert->getValidity().getNotBeforeTime() < min_issue_date) {
                min_issue_date = cert->getValidity().getNotBeforeTime();
            }
        }
    }

    return min_issue_date;
}

static time_t getEarliestExpirationDate(const CertificateChain* chain) {
    time_t min_expiration_date = 0;
    auto certs = chain->getCerts();

    if (!certs.empty()) {
        min_expiration_date = certs.front()->getValidity().getNotAfterTime();
        for (auto const& cert : certs) {
            if (cert->getValidity().getNotAfterTime() < min_expiration_date) {
                min_expiration_date = cert->getValidity().getNotAfterTime();
            }
        }
    }

    return min_expiration_date;
}

static time_t getLatestIssueDate(const CertificateChain* chain) {
    time_t max_issue_date = 0;
    auto certs = chain->getCerts();
    if (!certs.empty()) {
        max_issue_date = certs.front()->getValidity().getNotBeforeTime();
        for (auto const& cert : certs) {
            if (cert->getValidity().getNotBeforeTime() > max_issue_date) {
                max_issue_date = cert->getValidity().getNotBeforeTime();
            }
        }
    }
    return max_issue_date;
}

static time_t getLatestExpirationDate(const CertificateChain* chain) {
    time_t max_expiration_date = 0;
    auto certs = chain->getCerts();
    if (!certs.empty()) {
        max_expiration_date = certs.front()->getValidity().getNotAfterTime();
        for (auto const& cert : certs) {
            if (cert->getValidity().getNotAfterTime() > max_expiration_date) {
                max_expiration_date = cert->getValidity().getNotAfterTime();
            }
        }
    }
    return max_expiration_date;
}


/**
 * Helper function to return earliest & latest issue date and expiration date comparing all collaterals.
 * @param p_cert_chain_obj[IN] - Pointer to CertificateChain object containing PCK Cert chain (for quote with cert type 5, this should be extracted from the quote).
 * @param p_tcb_info_obj[IN] - Pointer to TcbInfo object.
 * @param p_quote_collateral[IN] - Pointer to _sgx_ql_qve_collateral_t struct.
 * @param p_earliest_issue_date[OUT] - Pointer to store the value of the earliest issue date of all input data in quote verification collaterals.
 * @param p_earliest_expiration_date[OUT] - Pointer to store the value of the earliest expiration date of all collaterals used in quote verification collaterals.
 * @param p_latest_issue_date[OUT] - Pointer to store the value of the latest issue date of all input data in quote verification collaterals.
 * @param p_latest_expiration_date[OUT] - Pointer to store the value of the latest expiration date of all collaterals used in quote verification collaterals.
 *
 * @return Status code of the operation, one of:
 *      - SGX_QL_SUCCESS
 *      - SGX_QL_ERROR_INVALID_PARAMETER
 *      - SGX_QL_ATT_KEY_CERT_DATA_INVALID
 *      - SGX_QL_QUOTE_CERTIFICATION_DATA_UNSUPPORTED
 *      - SGX_QL_ERROR_UNEXPECTED
 **/
static quote3_error_t qve_get_collateral_dates(const CertificateChain* p_cert_chain_obj, const json::TcbInfo* p_tcb_info_obj,
    const struct _sgx_ql_qve_collateral_t *p_quote_collateral, const char *crls[],
    time_t* p_earliest_issue_date, time_t* p_earliest_expiration_date,
    time_t* p_latest_issue_date, time_t* p_latest_expiration_date) {

    quote3_error_t ret = SGX_QL_ERROR_INVALID_PARAMETER;
    int version = 0;

    do {
        if (p_cert_chain_obj == NULL ||
            p_tcb_info_obj == NULL ||
            p_quote_collateral == NULL ||
            p_earliest_issue_date == NULL ||
            p_earliest_expiration_date == NULL ||
            p_latest_issue_date == NULL ||
            p_latest_expiration_date == NULL ||
            crls == NULL ||
            crls[0] == NULL ||
            crls[1] == NULL) {
            break;
        }
        *p_earliest_issue_date = 0;
        *p_earliest_expiration_date = 0;
        *p_latest_issue_date = 0;
        *p_latest_expiration_date = 0;

        CertificateChain qe_identity_issuer_chain;
        if (qe_identity_issuer_chain.parse((reinterpret_cast<const char*>(p_quote_collateral->qe_identity_issuer_chain))) != STATUS_OK) {
            ret = SGX_QL_PCK_CERT_CHAIN_ERROR;
            break;
        }
        CertificateChain tcb_info_issuer_chain;
        if (tcb_info_issuer_chain.parse((reinterpret_cast<const char*>(p_quote_collateral->tcb_info_issuer_chain))) != STATUS_OK) {
            ret = SGX_QL_PCK_CERT_CHAIN_ERROR;
            break;
        }

        EnclaveIdentityParser parser;
        std::unique_ptr<EnclaveIdentityV2> enclaveIdentity;
        try
        {
            enclaveIdentity = parser.parse(p_quote_collateral->qe_identity);
        }
        catch (...)
        {
            ret = SGX_QL_QEIDENTITY_UNSUPPORTED_FORMAT;
            break;
        }
        //supports only EnclaveIdentity V2 and V3
        //
        version = enclaveIdentity->getVersion();
        if (version != 2 && version != 3) {
            ret = SGX_QL_QEIDENTITY_UNSUPPORTED_FORMAT;
            break;
        }

        //supports only TCBInfo V2 and V3
        //
        version = p_tcb_info_obj->getVersion();
        if (version != 2 && version != 3) {
            ret = SGX_QL_TCBINFO_UNSUPPORTED_FORMAT;
            break;
        }

        pckparser::CrlStore root_ca_crl;
        if (root_ca_crl.parse(crls[0]) != true) {
            ret = SGX_QL_CRL_UNSUPPORTED_FORMAT;
            break;
        }

        pckparser::CrlStore pck_crl;
        if (pck_crl.parse(crls[1]) != true) {
            ret = SGX_QL_CRL_UNSUPPORTED_FORMAT;
            break;
        }

        CertificateChain pck_crl_issuer_chain;
        if (pck_crl_issuer_chain.parse((reinterpret_cast<const char*>(p_quote_collateral->pck_crl_issuer_chain))) != STATUS_OK) {
            ret = SGX_QL_PCK_CERT_CHAIN_ERROR;
            break;
        }

        //Earliest issue date
        //
        std::array <time_t, 8> earliest_issue;
        std::array <time_t, 8> earliest_expiration;
        std::array <time_t, 8> latest_issue;
        std::array <time_t, 8> latest_expiration;

        earliest_issue[0] = root_ca_crl.getValidity().notBeforeTime;
        earliest_issue[1] = pck_crl.getValidity().notBeforeTime;
        earliest_issue[2] = getEarliestIssueDate(&pck_crl_issuer_chain);
        earliest_issue[3] = getEarliestIssueDate(p_cert_chain_obj);
        earliest_issue[4] = getEarliestIssueDate(&tcb_info_issuer_chain);
        earliest_issue[5] = getEarliestIssueDate(&qe_identity_issuer_chain);
        earliest_issue[6] = p_tcb_info_obj->getIssueDate();
        earliest_issue[7] = enclaveIdentity->getIssueDate();

        earliest_expiration[0] = root_ca_crl.getValidity().notAfterTime;
        earliest_expiration[1] = pck_crl.getValidity().notAfterTime;
        earliest_expiration[2] = getEarliestExpirationDate(&pck_crl_issuer_chain);
        earliest_expiration[3] = getEarliestExpirationDate(p_cert_chain_obj);
        earliest_expiration[4] = getEarliestExpirationDate(&tcb_info_issuer_chain);
        earliest_expiration[5] = getEarliestExpirationDate(&qe_identity_issuer_chain);
        earliest_expiration[6] = p_tcb_info_obj->getNextUpdate();
        earliest_expiration[7] = enclaveIdentity->getNextUpdate();

        latest_issue[0] = root_ca_crl.getValidity().notBeforeTime;
        latest_issue[1] = pck_crl.getValidity().notBeforeTime;
        latest_issue[2] = getLatestIssueDate(&pck_crl_issuer_chain);
        latest_issue[3] = getLatestIssueDate(p_cert_chain_obj);
        latest_issue[4] = getLatestIssueDate(&tcb_info_issuer_chain);
        latest_issue[5] = getLatestIssueDate(&qe_identity_issuer_chain);
        latest_issue[6] = p_tcb_info_obj->getIssueDate();
        latest_issue[7] = enclaveIdentity->getIssueDate();
        latest_expiration[0] = root_ca_crl.getValidity().notAfterTime;
        latest_expiration[1] = pck_crl.getValidity().notAfterTime;
        latest_expiration[2] = getLatestExpirationDate(&pck_crl_issuer_chain);
        latest_expiration[3] = getLatestExpirationDate(p_cert_chain_obj);
        latest_expiration[4] = getLatestExpirationDate(&tcb_info_issuer_chain);
        latest_expiration[5] = getLatestExpirationDate(&qe_identity_issuer_chain);
        latest_expiration[6] = p_tcb_info_obj->getNextUpdate();
        latest_expiration[7] = enclaveIdentity->getNextUpdate();


        //p_earliest_issue_date
        //
        *p_earliest_issue_date = *std::min_element(earliest_issue.begin(), earliest_issue.end());

        //p_earliest_expiration_date
        //
        *p_earliest_expiration_date = *std::min_element(earliest_expiration.begin(), earliest_expiration.end());

        //p_latest_issue_date
        //
        *p_latest_issue_date = *std::max_element(latest_issue.begin(), latest_issue.end());

        //p_latest_expiration_date
        //
        *p_latest_expiration_date = *std::max_element(latest_expiration.begin(), latest_expiration.end());

        if (*p_earliest_issue_date == 0 || *p_earliest_expiration_date == 0 ||
            *p_latest_issue_date == 0 || *p_latest_expiration_date == 0) {
            ret = SGX_QL_ERROR_UNEXPECTED;
        }
        ret = SGX_QL_SUCCESS;
    } while (0);

    return ret;
}

/**
 * Setup supplemental data.
 * @param chain[IN] - Pointer to CertificateChain object containing PCK Cert chain (for quote with cert type 5, this should be extracted from the quote).
 * @param tcb_info_obj[IN] - Pointer to TcbInfo object.
 * @param p_quote_collateral[IN] - Pointer to _sgx_ql_qve_collateral_t struct.
 * @param earliest_issue_date[IN] - value of the earliest issue date of all collaterals used in quote verification.
 * @param p_supplemental_data[OUT] - Pointer to a supplemental data buffer. Must be allocated by caller (untrusted code).

 * @return Status code of the operation, one of:
 *      - SGX_QL_SUCCESS
 *      - SGX_QL_ERROR_INVALID_PARAMETER
 *      - SGX_QL_TCBINFO_UNSUPPORTED_FORMAT
 *      - SGX_QL_PCK_CERT_CHAIN_ERROR
 *      - SGX_QL_ATT_KEY_CERT_DATA_INVALID
 *      - SGX_QL_QUOTE_CERTIFICATION_DATA_UNSUPPORTED
 *      - SGX_QL_ERROR_UNEXPECTED
 **/
static quote3_error_t qve_set_quote_supplemental_data(const CertificateChain *chain, const json::TcbInfo *tcb_info_obj,
    uint16_t qe_report_isvsvn, const struct _sgx_ql_qve_collateral_t *p_quote_collateral, const char *crls[],
    time_t earliest_issue_date, time_t latest_issue_date, time_t earliest_expiration_date,
    uint8_t *p_supplemental_data) {
    if (chain == NULL ||
        tcb_info_obj == NULL ||
        p_quote_collateral == NULL ||
        crls == NULL ||
        crls[0] == NULL ||
        crls[1] == NULL ||
        p_supplemental_data == NULL) {
        return SGX_QL_ERROR_INVALID_PARAMETER;
    }

    quote3_error_t ret = SGX_QL_ERROR_INVALID_PARAMETER;
    int version = 0;
    sgx_ql_qv_supplemental_t* supplemental_data = reinterpret_cast<sgx_ql_qv_supplemental_t*> (p_supplemental_data);

    //Set default values
    memset_s(supplemental_data, sizeof(*supplemental_data), 0, sizeof(*supplemental_data));
    supplemental_data->dynamic_platform = PCK_FLAG_UNDEFINED;
    supplemental_data->cached_keys = PCK_FLAG_UNDEFINED;
    supplemental_data->smt_enabled = PCK_FLAG_UNDEFINED;

    time_t qe_identity_date = 0;
    //Start collecting supplemental data
    //
    do {
        EnclaveIdentityParser parser;
        EnclaveIdentityV2* qe_identity_v2 = NULL;

        //some of the required supplemental data exist only on V2 & V3 TCBInfo, validate TCBInfo version.
        //
        version = tcb_info_obj->getVersion();
        if (version != 2 && version != 3) {
            ret = SGX_QL_TCBINFO_UNSUPPORTED_FORMAT;
            break;
        }

        //parse qe_identity and validate its version
        //
        std::unique_ptr<EnclaveIdentityV2> qe_identity_obj;
        try
        {
            qe_identity_obj = parser.parse(p_quote_collateral->qe_identity);
        }
        catch (...)
        {
            ret = SGX_QL_QEIDENTITY_UNSUPPORTED_FORMAT;
            break;
        }
        if (qe_identity_obj == nullptr) {
            ret = SGX_QL_QEIDENTITY_UNSUPPORTED_FORMAT;
            break;
        }

        version = qe_identity_obj->getVersion();
        if (version == 2 || version == 3) {
            qe_identity_v2 = dynamic_cast<EnclaveIdentityV2*>(qe_identity_obj.get());
        }
        else {
            ret = SGX_QL_QEIDENTITY_UNSUPPORTED_FORMAT;
            break;
        }

        pckparser::CrlStore root_ca_crl;
        if (root_ca_crl.parse(crls[0]) != true) {
            ret = SGX_QL_ERROR_INVALID_PARAMETER;
            break;
        }

        pckparser::CrlStore pck_crl;
        if (pck_crl.parse(crls[1]) != true) {
            ret = SGX_QL_ERROR_INVALID_PARAMETER;
            break;
        }

        //get certificates objects from chain
        //
        auto chain_root_ca_cert = chain->getRootCert();
        auto chain_pck_cert = chain->getPckCert();

        //compare to nullptr (C++ way of comparing pointers to NULL), otherwise gcc will report a compilation warning
        //
        if (chain_root_ca_cert == nullptr || chain_pck_cert == nullptr) {
            ret = SGX_QL_PCK_CERT_CHAIN_ERROR;
            break;
        }
        auto pck_cert_tcb = chain_pck_cert->getTcb();

        supplemental_data->version = SUPPLEMENTAL_DATA_VERSION;
        supplemental_data->earliest_issue_date = earliest_issue_date;
        supplemental_data->latest_issue_date = latest_issue_date;
        supplemental_data->earliest_expiration_date = earliest_expiration_date;
        supplemental_data->tcb_level_date_tag = 0;

        //get matching QE identity TCB level
        //
        auto qe_identity_tcb_levels = qe_identity_v2->getTcbLevels();

        //make sure QE identity has at least one TCBLevel
        //
        if (qe_identity_tcb_levels.empty()) {
            ret = SGX_QL_QEIDENTITY_UNSUPPORTED_FORMAT;
            break;
        }
        for (const auto & tcbLevel : qe_identity_tcb_levels) {
            if (tcbLevel.getIsvsvn() <= qe_report_isvsvn) {
                tm matching_qe_identity_tcb_date = tcbLevel.getTcbDate();
                qe_identity_date = intel::sgx::dcap::mktime(&matching_qe_identity_tcb_date);
                break;
            }
        }

        //get TCB date of TCB level in TCB Info
        //
        auto matching_tcb_info_tcb_date = getMatchingTcbLevelTcbDate(tcb_info_obj->getTcbLevels(), *chain_pck_cert);

        //make sure none of TCBLevel dates is 0
        //
        if (qe_identity_date < 0 || matching_tcb_info_tcb_date < 0) {
            ret = SGX_QL_ERROR_UNEXPECTED;
            break;
        }
        //compare TCB info TCB level date and QE identity TCB level date, return the smaller one
        //
        if (qe_identity_date <= matching_tcb_info_tcb_date) {
            supplemental_data->tcb_level_date_tag = qe_identity_date;
        }
        else {
            supplemental_data->tcb_level_date_tag = matching_tcb_info_tcb_date;
        }

        //make sure that long int value returned in getCrlNum doesn't overflow
        //
        long tmp_crl_num = pck_crl.getCrlNum();
        if (tmp_crl_num > UINT32_MAX || tmp_crl_num < 0) {
            ret = SGX_QL_ERROR_UNEXPECTED;
            break;
        }
        supplemental_data->pck_crl_num = (uint32_t)tmp_crl_num;

        //make sure that long int value returned in getCrlNum doesn't overflow
        //
        tmp_crl_num = root_ca_crl.getCrlNum();
        if (tmp_crl_num > UINT32_MAX || tmp_crl_num < 0) {
            ret = SGX_QL_ERROR_UNEXPECTED;
            break;
        }

        supplemental_data->root_ca_crl_num = (uint32_t)tmp_crl_num;

        if (qe_identity_v2->getTcbEvaluationDataNumber() <= tcb_info_obj->getTcbEvaluationDataNumber()) {
            supplemental_data->tcb_eval_ref_num = qe_identity_v2->getTcbEvaluationDataNumber();
        }
        else {
            supplemental_data->tcb_eval_ref_num = tcb_info_obj->getTcbEvaluationDataNumber();
        }
        // generates SHA-384 hash of CERT chain root CA's public key
        //
        const uint8_t* root_pub_key = chain_root_ca_cert->getPubKey().data();
        size_t root_pub_key_size = chain_root_ca_cert->getPubKey().size();
        if (SHA384((const unsigned char *)root_pub_key, root_pub_key_size, supplemental_data->root_key_id) == NULL) {
            ret = SGX_QL_ERROR_UNEXPECTED;
            break;
        }

        //get PPID value from PCK Cert
        //
        auto pck_cert_ppid = chain_pck_cert->getPpid();

        //validate PPID buffer size
        //
        if (sizeof(sgx_key_128bit_t) != pck_cert_ppid.size()) {
            ret = SGX_QL_PCK_CERT_UNSUPPORTED_FORMAT;
            break;
        }

        //copy PPID value into supplemental data buffer
        //
        if (memcpy_s(supplemental_data->pck_ppid, sizeof(sgx_key_128bit_t),
            pck_cert_ppid.data(), pck_cert_ppid.size()) != 0) {
            ret = SGX_QL_ERROR_UNEXPECTED;
            break;
        }

        //get CpuSvn value from PCK Cert
        //
        auto pck_cert_tcb_cpusvn = pck_cert_tcb.getCpuSvn();

        //validate CpuSvn buffer size
        //
        if (sizeof(supplemental_data->tcb_cpusvn.svn) != pck_cert_tcb_cpusvn.size()) {
            ret = SGX_QL_PCK_CERT_UNSUPPORTED_FORMAT;
            break;
        }

        //copy CpuSvn value into supplemental data buffer
        //
        if (memcpy_s(supplemental_data->tcb_cpusvn.svn, sizeof(supplemental_data->tcb_cpusvn.svn),
            pck_cert_tcb_cpusvn.data(), pck_cert_tcb_cpusvn.size()) != 0) {
            ret = SGX_QL_ERROR_UNEXPECTED;
            break;
        }

        //make sure unsigned int value returned in getPceSvn matches uint16_t size, prevent overflow
        //
        unsigned int tmp_pce_svn = pck_cert_tcb.getPceSvn();
        if (tmp_pce_svn > UINT16_MAX) {
            ret = SGX_QL_ERROR_UNEXPECTED;
            break;
        }

        supplemental_data->tcb_pce_isvsvn = (sgx_isv_svn_t)pck_cert_tcb.getPceSvn();
        supplemental_data->pce_id = *(chain_pck_cert->getPceId().data());

        x509::SgxType tmp_sgx_type = chain_pck_cert->getSgxType();
        if (tmp_sgx_type > UINT8_MAX) {
            ret = SGX_QL_ERROR_UNEXPECTED;
            break;
        }

        supplemental_data->sgx_type = (uint8_t)tmp_sgx_type;


        //try to get flags for multi-package platforms
        //
        if (supplemental_data->sgx_type == x509::Scalable || supplemental_data->sgx_type == x509::ScalableWithIntegrity) {
            try {
                auto pck_cert = chain->getTopmostCert();
                auto platform_cert = x509::PlatformPckCertificate(*pck_cert);


                //get platform instance ID from PCK Cert
                //
                auto platform_instance_id = platform_cert.getPlatformInstanceId();

                //copy platform instance ID value into supplemental data buffer
                //
                if (memcpy_s(supplemental_data->platform_instance_id, 16,
                    platform_instance_id.data(), platform_instance_id.size()) != 0) {
                    ret = SGX_QL_ERROR_UNEXPECTED;
                    break;
                }

                //get configuration data from PCK Cert
                //
                auto sgx_configuration = platform_cert.getConfiguration();

                if (sgx_configuration.isDynamicPlatform())
                    supplemental_data->dynamic_platform = PCK_FLAG_TRUE;

                if (sgx_configuration.isCachedKeys())
                    supplemental_data->cached_keys = PCK_FLAG_TRUE;

                if (sgx_configuration.isSmtEnabled())
                    supplemental_data->smt_enabled = PCK_FLAG_TRUE;
                else
                    supplemental_data->smt_enabled = PCK_FLAG_FALSE;
            }
            catch (...) {
                ret = SGX_QL_PCK_CERT_UNSUPPORTED_FORMAT;
                break;
            }
        }

        ret = SGX_QL_SUCCESS;
    } while (0);

    if (ret != SGX_QL_SUCCESS) {
        memset_s(supplemental_data, sizeof(*supplemental_data), 0, sizeof(*supplemental_data));
        supplemental_data->dynamic_platform = PCK_FLAG_UNDEFINED;
        supplemental_data->cached_keys = PCK_FLAG_UNDEFINED;
        supplemental_data->smt_enabled = PCK_FLAG_UNDEFINED;
    }

    return ret;
}


/**
 * Get supplemental data required size.
 * @param p_data_size[OUT] - Pointer to hold the size of the buffer in bytes required to contain all of the supplemental data.
 *
 * @return Status code of the operation, one of:
 *      - SGX_QL_SUCCESS
 *      - SGX_QL_ERROR_INVALID_PARAMETER
 **/
quote3_error_t sgx_qve_get_quote_supplemental_data_size(
    uint32_t *p_data_size) {
    if (p_data_size == NULL ||
        (sgx_is_within_enclave(p_data_size, sizeof(*p_data_size)) == 0)) {
        return SGX_QL_ERROR_INVALID_PARAMETER;
    }
    *p_data_size = sizeof(sgx_ql_qv_supplemental_t);
    return SGX_QL_SUCCESS;
}


/**
 * Get supplemental data version.
 * @param p_version[OUT] - Pointer to hold the version of the supplemental data.
 *
 * @return Status code of the operation, one of:
 *      - SGX_QL_SUCCESS
 *      - SGX_QL_ERROR_INVALID_PARAMETER
 **/
quote3_error_t sgx_qve_get_quote_supplemental_data_version(
    uint32_t *p_version) {
    if (p_version == NULL ||
        (sgx_is_within_enclave(p_version, sizeof(*p_version)) == 0)) {
        return SGX_QL_ERROR_INVALID_PARAMETER;
    }
    *p_version = SUPPLEMENTAL_DATA_VERSION;
    return SGX_QL_SUCCESS;
}


#ifdef SGX_TRUSTED
/**
 * Generate enclave report with:
 * SHA256([nonce || quote || expiration_check_date || expiration_status || verification_result || supplemental_data] || 32 - 0x00s)
 *
 * @param p_quote[IN] - Pointer to an SGX Quote.
 * @param quote_size[IN] - Size of the buffer pointed to by p_quote (in bytes).
 * @param expiration_check_date[IN] - This is the date that the QvE will use to determine if any of the inputted collateral have expired.
 * @param p_collateral_expiration_status[IN] - Address of the outputted expiration status.
 * @param p_quote_verification_result[IN] - Address of the outputted quote verification result.
 * @param p_qve_report_info[IN/OUT] - QvE will generate a report using the target_info provided in the sgx_ql_qe_report_info_t structure, and store it in qe_report.
 * @param supplemental_data_size[IN] - Size of the buffer pointed to by p_supplemental_data (in bytes).
 * @param p_supplemental_data[IN] - Buffer containing supplemental data.
 *
 * @return Status code of the operation, one of:
 *      - SGX_QL_SUCCESS
 *      - SGX_QL_ERROR_INVALID_PARAMETER
 *      - SGX_QL_UNABLE_TO_GENERATE_REPORT
 **/
static quote3_error_t sgx_qve_generate_report(
    const uint8_t *p_quote,
    uint32_t quote_size,
    const time_t expiration_check_date,
    uint32_t *p_collateral_expiration_status,
    sgx_ql_qv_result_t *p_quote_verification_result,
    sgx_ql_qe_report_info_t *p_qve_report_info,
    uint32_t supplemental_data_size,
    uint8_t *p_supplemental_data)
{

    //validate parameters
    //
    if (p_quote == NULL ||
        quote_size < QUOTE_MIN_SIZE ||
        p_collateral_expiration_status == NULL ||
        p_qve_report_info == NULL ||
        expiration_check_date == 0 ||
        p_quote_verification_result == NULL ||
        CHECK_OPT_PARAMS(p_supplemental_data, supplemental_data_size)) {
        return SGX_QL_ERROR_INVALID_PARAMETER;
    }

    sgx_status_t sgx_status = SGX_ERROR_UNEXPECTED;
    quote3_error_t ret = SGX_QL_UNABLE_TO_GENERATE_REPORT;
    sgx_sha_state_handle_t sha_handle = NULL;
    sgx_report_data_t report_data = { 0 };


    do {
        //Create QvE report
        //
        //report_data = SHA256([nonce || quote || expiration_check_date || expiration_status || verification_result || supplemental_data]) || 32 - 0x00s
        //
        sgx_status = sgx_sha256_init(&sha_handle);
        SGX_ERR_BREAK(sgx_status);

        //nonce
        //
        sgx_status = sgx_sha256_update((p_qve_report_info->nonce.rand), sizeof(p_qve_report_info->nonce.rand), sha_handle);
        SGX_ERR_BREAK(sgx_status);

        //quote
        //
        sgx_status = sgx_sha256_update(p_quote, quote_size, sha_handle);
        SGX_ERR_BREAK(sgx_status);

        //expiration_check_date
        //
        sgx_status = sgx_sha256_update((const uint8_t*)&expiration_check_date, sizeof(expiration_check_date), sha_handle);
        SGX_ERR_BREAK(sgx_status);

        //p_collateral_expiration_status
        //
        sgx_status = sgx_sha256_update((const uint8_t*)p_collateral_expiration_status, sizeof(*p_collateral_expiration_status), sha_handle);
        SGX_ERR_BREAK(sgx_status);


        //p_quote_verification_result
        //
        sgx_status = sgx_sha256_update((uint8_t *)const_cast<sgx_ql_qv_result_t *>(p_quote_verification_result), sizeof(*p_quote_verification_result), sha_handle);
        SGX_ERR_BREAK(sgx_status);


        //p_supplemental_data
        //
        if (p_supplemental_data) {
            sgx_status = sgx_sha256_update(p_supplemental_data, supplemental_data_size, sha_handle);
            SGX_ERR_BREAK(sgx_status);
        }

        //get the hashed report_data
        //
        sgx_status = sgx_sha256_get_hash(sha_handle, reinterpret_cast<sgx_sha256_hash_t *>(&report_data));
        SGX_ERR_BREAK(sgx_status);

        //create QVE report with report_data embedded
        //
        sgx_status = sgx_create_report(&(p_qve_report_info->app_enclave_target_info), &report_data, &(p_qve_report_info->qe_report));
        SGX_ERR_BREAK(sgx_status);

        ret = SGX_QL_SUCCESS;
    } while (0);

    //clear data in report_data (it's a local variable, no need for free).
    //
    memset_s(&report_data, sizeof(sgx_report_data_t), 0, sizeof(sgx_report_data_t));
    if (sha_handle != NULL) {
        sgx_sha256_close(sha_handle);
    }
    return ret;
}
#endif //SGX_TRUSTED

#define IS_IN_ENCLAVE_POINTER(p, size) (p && (strnlen(p, size) == size - 1) && sgx_is_within_enclave(p, size))

//CRL may DER encoding, so don't use to strnlen to check the length
static bool is_collateral_deep_copied(const struct _sgx_ql_qve_collateral_t *p_quote_collateral) {
    if (IS_IN_ENCLAVE_POINTER(p_quote_collateral->pck_crl_issuer_chain, p_quote_collateral->pck_crl_issuer_chain_size) &&
        IS_IN_ENCLAVE_POINTER(p_quote_collateral->tcb_info_issuer_chain, p_quote_collateral->tcb_info_issuer_chain_size) &&
        IS_IN_ENCLAVE_POINTER(p_quote_collateral->tcb_info, p_quote_collateral->tcb_info_size) &&
        IS_IN_ENCLAVE_POINTER(p_quote_collateral->qe_identity_issuer_chain, p_quote_collateral->qe_identity_issuer_chain_size) &&
        IS_IN_ENCLAVE_POINTER(p_quote_collateral->qe_identity, p_quote_collateral->qe_identity_size) &&
        sgx_is_within_enclave(p_quote_collateral->root_ca_crl, p_quote_collateral->root_ca_crl_size) &&
        sgx_is_within_enclave(p_quote_collateral->pck_crl, p_quote_collateral->pck_crl_size)) {
        return true;
    }
    else {
        return false;
    }
}
/**
 * Perform quote verification.
 *
 * @param p_quote[IN] - Pointer to an SGX Quote.
 * @param quote_size[IN] - Size of the buffer pointed to by p_quote (in bytes).
 * @param p_quote_collateral[IN] - This is a pointer to the Quote Certification Collateral provided by the caller.
 * @param expiration_check_date[IN] - This is the date that the QvE will use to determine if any of the inputted collateral have expired.
 * @param p_collateral_expiration_status[OUT] - Address of the outputted expiration status.  This input must not be NULL.
 * @param p_quote_verification_result[OUT] - Address of the outputted quote verification result.
 * @param p_qve_report_info[IN/OUT] - This parameter is optional.  If not NULL, the QvE will generate a report with using the target_info provided in the sgx_ql_qe_report_info_t structure.
 * @param supplemental_data_size[IN] - Size of the buffer pointed to by p_supplemental_data (in bytes).
 * @param p_supplemental_data[OUT] - The parameter is optional.  If it is NULL, supplemental_data_size must be 0.
 *
 * @return Status code of the operation, one of:
 *      - SGX_QL_SUCCESS
 *      - SGX_QL_ERROR_INVALID_PARAMETER
 *      - SGX_QL_QUOTE_FORMAT_UNSUPPORTED
 *      - SGX_QL_QUOTE_CERTIFICATION_DATA_UNSUPPORTED
 *      - SGX_QL_UNABLE_TO_GENERATE_REPORT
 *      - SGX_QL_CRL_UNSUPPORTED_FORMAT
 *      - SGX_QL_ERROR_UNEXPECTED
 **/
quote3_error_t sgx_qve_verify_quote(
    const uint8_t *p_quote,
    uint32_t quote_size,
    const struct _sgx_ql_qve_collateral_t *p_quote_collateral,
    const time_t expiration_check_date,
    uint32_t *p_collateral_expiration_status,
    sgx_ql_qv_result_t *p_quote_verification_result,
    sgx_ql_qe_report_info_t *p_qve_report_info,
    uint32_t supplemental_data_size,
    uint8_t *p_supplemental_data) {

    //validate result parameter pointers and set default values
    //in case of any invalid result parameter, set outputs_set = 0 and then return invalid (after setting
    //default values of valid result parameters)
    //
    bool outputs_set = 1;
    if (p_collateral_expiration_status &&
        sgx_is_within_enclave(p_collateral_expiration_status, sizeof(*p_collateral_expiration_status))) {
        *p_collateral_expiration_status = 1;
    }
    else {
        outputs_set = 0;
    }

    if (p_quote_verification_result &&
        sgx_is_within_enclave(p_quote_verification_result, sizeof(*p_quote_verification_result))) {
        *p_quote_verification_result = SGX_QL_QV_RESULT_UNSPECIFIED;
    }
    else {
        outputs_set = 0;
    }

    //check if supplemental data required, and validate its size matches sgx_ql_qv_supplemental_t struct. if so, set it to 0 before
    //
    if (p_supplemental_data) {
        if (supplemental_data_size == sizeof(sgx_ql_qv_supplemental_t) &&
            sgx_is_within_enclave(p_supplemental_data, supplemental_data_size)) {
            memset_s(p_supplemental_data, supplemental_data_size, 0, supplemental_data_size);
        }
        else {
            outputs_set = 0;
        }
    }
    if (outputs_set == 0) {
        return SGX_QL_ERROR_INVALID_PARAMETER;
    }

    //validate collateral version
    //
    if (p_quote_collateral->version != QVE_COLLATERAL_VERSION1 &&
         p_quote_collateral->version != QVE_COLLATERAL_VERSION3 &&
         p_quote_collateral->version != QVE_COLLATERAL_VERSOIN31 &&
         p_quote_collateral->version != QVE_COLLATERAL_VERSION4) {

        return SGX_QL_COLLATERAL_VERSION_NOT_SUPPORTED;
    }

    //validate parameters
    //
    if (p_quote == NULL ||
        quote_size < QUOTE_MIN_SIZE ||
        !sgx_is_within_enclave(p_quote, quote_size) ||
        p_quote_collateral == NULL ||
        !sgx_is_within_enclave(p_quote_collateral, sizeof(*p_quote_collateral)) ||
        !is_collateral_deep_copied(p_quote_collateral) ||
        expiration_check_date <= 0 ||
        (p_qve_report_info != NULL && !sgx_is_within_enclave(p_qve_report_info, sizeof(*p_qve_report_info))) ||
        (p_supplemental_data == NULL && supplemental_data_size != 0)) {
        //one or more invalid parameters
        //
        return SGX_QL_ERROR_INVALID_PARAMETER;
    }

    //define local variables
    //
    time_t earliest_expiration_date = 0;
    time_t earliest_issue_date = 0;
    time_t latest_expiration_date = 0;
    time_t latest_issue_date = 0;
    Status collateral_verification_res = STATUS_SGX_ENCLAVE_REPORT_MRSIGNER_MISMATCH;
    quote3_error_t ret = SGX_QL_ERROR_INVALID_PARAMETER;
    uint32_t pck_cert_chain_size = 0;
    uint8_t *p_pck_cert_chain = NULL;
    time_t set_time = 0;
    CertificateChain chain;
    json::TcbInfo tcb_info_obj;
    std::vector<uint8_t> hardcode_root_pub_key;
    std::string root_cert_str;
    std::string root_crl;
    std::string pck_crl;

    //start the verification operation
    //
    do {
        //setup expiration check date to verify against (trusted time)
        //
        set_time = intel::sgx::dcap::getCurrentTime(&expiration_check_date);

        // defense-in-depth to make sure current time is set as expected.
        //
        if (set_time != expiration_check_date) {
            ret = SGX_QL_ERROR_UNEXPECTED;
            break;
        }

        //extract PCK Cert chain from the given quote
        //
        ret = extract_chain_from_quote(p_quote, quote_size, &pck_cert_chain_size, &p_pck_cert_chain);
        if (ret != SGX_QL_SUCCESS || !p_pck_cert_chain) {
            break;
        }

        try
        {
            //parse tcbInfo JSON string into TcbInfo object
            //
            tcb_info_obj = json::TcbInfo::parse(p_quote_collateral->tcb_info);
        }
        catch (...)
        {
            //unable to parse tcbInfo JSON, return an error
            //
            ret = status_error_to_quote3_error(STATUS_SGX_TCB_INFO_INVALID);
            break;
        }

        //parse PCK Cert chain into CertificateChain object. return error in case of failure
        //
        if (chain.parse((reinterpret_cast<const char*>(p_pck_cert_chain))) != STATUS_OK ||
            chain.length() != EXPECTED_CERTIFICATE_COUNT_IN_PCK_CHAIN) {
            ret = SGX_QL_PCK_CERT_CHAIN_ERROR;
            break;
        }

        //if user provide DER encoding Root CA CRL, try to convert it to hex encoding
        //
        if (!check_pem_crl(p_quote_collateral->root_ca_crl, p_quote_collateral->root_ca_crl_size)) {
            if (!check_hex_crl(p_quote_collateral->root_ca_crl, p_quote_collateral->root_ca_crl_size)) {

                root_crl = bin2hex(p_quote_collateral->root_ca_crl, p_quote_collateral->root_ca_crl_size);

                if (root_crl.empty())
                    break;
            }
        }

        //if user provide DER encoding PCK CRL, try to convert it to hex encoding
        //
        if (!check_pem_crl(p_quote_collateral->pck_crl, p_quote_collateral->pck_crl_size)) {
            if (!check_hex_crl(p_quote_collateral->pck_crl, p_quote_collateral->pck_crl_size)) {

                pck_crl = bin2hex(p_quote_collateral->pck_crl, p_quote_collateral->pck_crl_size);

                if (pck_crl.empty())
                    break;
            }
        }

        //create CRLs combined array
        //
        std::array<const char*, 2> crls;

        if (!root_crl.empty())
            crls[0] = root_crl.c_str();
        else
            crls[0] = p_quote_collateral->root_ca_crl;

        if (!pck_crl.empty())
            crls[1] = pck_crl.c_str();
        else
            crls[1] = p_quote_collateral->pck_crl;


        //extract root CA from PCK cert chain in quote
        auto root_cert = chain.getRootCert();
        x509::Certificate root_cert_x509;

        //compare to nullptr (C++ way of comparing pointers to NULL), otherwise gcc will report a compilation warning
        //
        if (root_cert == nullptr) {
            ret = SGX_QL_PCK_CERT_CHAIN_ERROR;
            break;
        }

        try {
            root_cert_x509 = x509::Certificate(*root_cert);
        }
        catch (...) {
            ret = SGX_QL_PCK_CERT_UNSUPPORTED_FORMAT;
            break;
        }

        auto root_pub_key_from_cert = root_cert_x509.getPubKey();

        std::copy(std::begin(INTEL_ROOT_PUB_KEY), std::end(INTEL_ROOT_PUB_KEY), std::back_inserter(hardcode_root_pub_key));

        //check root public key
        //
        if (hardcode_root_pub_key != root_pub_key_from_cert) {
            ret = SGX_QL_PCK_CERT_CHAIN_ERROR;
            break;
        }

        //convert root cert to string
        //
        root_cert_str = root_cert_x509.getPem();
        if (root_cert_str.empty()) {
            ret = SGX_QL_PCK_CERT_CHAIN_ERROR;
            break;
        }

        ret = qve_get_collateral_dates(&chain, &tcb_info_obj,
            p_quote_collateral, crls.data(),
            &earliest_issue_date, &earliest_expiration_date,
            &latest_issue_date, &latest_expiration_date);
        if (ret != SGX_QL_SUCCESS) {
            break;
        }

        //update collateral expiration status
        //
        if (earliest_expiration_date <= expiration_check_date) {
            *p_collateral_expiration_status = 1;
        }
        else {
            *p_collateral_expiration_status = 0;
        }

        //parse and verify PCK certificate chain
        //
        collateral_verification_res = sgxAttestationVerifyPCKCertificate((const char*)p_pck_cert_chain, crls.data(), root_cert_str.c_str(), &expiration_check_date);
        if (collateral_verification_res != STATUS_OK) {
            if (is_expiration_error(collateral_verification_res)) {
                *p_collateral_expiration_status = 1;
            }
            else {
                ret = status_error_to_quote3_error(collateral_verification_res);
                break;
            }
        }

        //parse and verify TCB info
        //
        collateral_verification_res = sgxAttestationVerifyTCBInfo(p_quote_collateral->tcb_info, p_quote_collateral->tcb_info_issuer_chain, crls[0], root_cert_str.c_str(), &expiration_check_date);
        if (collateral_verification_res != STATUS_OK) {
            if (is_expiration_error(collateral_verification_res)) {
                *p_collateral_expiration_status = 1;
            }
            else {
                ret = status_error_to_quote3_error(collateral_verification_res);
                break;
            }
        }

        //parse and verify QE identity
        //
        collateral_verification_res = sgxAttestationVerifyEnclaveIdentity(p_quote_collateral->qe_identity, p_quote_collateral->qe_identity_issuer_chain, crls[0], root_cert_str.c_str(), &expiration_check_date);
        if (collateral_verification_res != STATUS_OK) {
            if (is_expiration_error(collateral_verification_res)) {
                *p_collateral_expiration_status = 1;
            }
            else {
                ret = status_error_to_quote3_error(collateral_verification_res);
                break;
            }
        }

        //parse and verify the quote, update verification results
        //
        collateral_verification_res = sgxAttestationVerifyQuote(p_quote, quote_size, chain.getPckCert()->getPem().c_str(), crls[1], p_quote_collateral->tcb_info, p_quote_collateral->qe_identity);
        *p_quote_verification_result = status_error_to_ql_qve_result(collateral_verification_res);

        if (is_nonterminal_error(collateral_verification_res)) {
            ret = SGX_QL_SUCCESS;
        }
        else {
            ret = status_error_to_quote3_error(collateral_verification_res);
        }

        //collect supplemental data if required, only if verification completed with non-terminal status
        //
        if (p_supplemental_data && ret == SGX_QL_SUCCESS) {
            // We totaly trust user on this, it should be explicitly and clearly
            // mentioned in doc, is there any max quote len other than numeric_limit<uint32_t>::max() ?
            const std::vector<uint8_t> vecQuote(p_quote, std::next(p_quote, quote_size));

            Quote quote;
            if (!quote.parse(vecQuote))
            {
                ret = status_error_to_quote3_error(STATUS_UNSUPPORTED_QUOTE_FORMAT);
            }
            auto qe_report_isvsvn = quote.getQeReport().isvSvn;
            ret = qve_set_quote_supplemental_data(&chain, &tcb_info_obj, qe_report_isvsvn, p_quote_collateral, crls.data(), earliest_issue_date, latest_issue_date, earliest_expiration_date, p_supplemental_data);
            if (ret != SGX_QL_SUCCESS) {
                break;
            }
        }

    } while (0);


#ifdef SGX_TRUSTED

    //defense-in-depth: validate that input current_time still returned by getCurrentTime
    //
    if (ret == SGX_QL_SUCCESS && set_time != getCurrentTime(&expiration_check_date)) {
        ret = SGX_QL_ERROR_UNEXPECTED;
    }

    //check if report is required
    //
    if (p_qve_report_info != NULL && ret == SGX_QL_SUCCESS) {

        quote3_error_t generate_report_ret = SGX_QL_ERROR_INVALID_PARAMETER;

        //generate a report with the verification result and input collaterals
        //
        generate_report_ret = sgx_qve_generate_report(
            p_quote,
            quote_size,
            expiration_check_date,
            p_collateral_expiration_status,
            p_quote_verification_result,
            p_qve_report_info,
            supplemental_data_size,
            p_supplemental_data);
        if (generate_report_ret != SGX_QL_SUCCESS) {
            ret = generate_report_ret;
            memset_s(&(p_qve_report_info->qe_report), sizeof(p_qve_report_info->qe_report), 0, sizeof(p_qve_report_info->qe_report));
        }
    }
 #endif //SGX_TRUSTED

    //clear and free allocated memory
    //
    if (p_pck_cert_chain) {
        CLEAR_FREE_MEM(p_pck_cert_chain, pck_cert_chain_size);
    }

    //if any check or operation failed (e.g. generating report, or supplemental data)
    //set p_quote_verification_result to SGX_QL_QV_RESULT_UNSPECIFIED
    //
    if (ret != SGX_QL_SUCCESS) {
        *p_quote_verification_result = SGX_QL_QV_RESULT_UNSPECIFIED;
    }

    return ret;
}

