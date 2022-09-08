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

#ifndef SGX_DCAP_PARSERS_PARSER_UTILS_H
#define SGX_DCAP_PARSERS_PARSER_UTILS_H

class Type;

#include "SgxEcdsaAttestation/AttestationParsers.h"

#include <openssl/ossl_typ.h>

#include <cstring>
#include <string>
#include <map>
#include <vector>

namespace intel { namespace sgx { namespace dcap { namespace parser {

std::string obj2Str(const ASN1_OBJECT* obj);
std::vector<uint8_t> bn2Vec(const BIGNUM* bn);
std::string x509NameToString(const X509_NAME* name);
std::string getNameEntry(X509_NAME* name, int nid);
std::tuple<time_t, time_t> asn1TimePeriodToCTime(const ASN1_TIME* validityBegin, const ASN1_TIME* validityEnd);
std::string getLastError();

namespace oids {

const std::string SGX_EXTENSION = "1.2.840.113741.1.13.1";
const std::string TCB = SGX_EXTENSION + ".2";
const std::string PPID = SGX_EXTENSION + ".1";
const std::string SGX_TCB_COMP01_SVN = TCB + ".1";
const std::string SGX_TCB_COMP02_SVN = TCB + ".2";
const std::string SGX_TCB_COMP03_SVN = TCB + ".3";
const std::string SGX_TCB_COMP04_SVN = TCB + ".4";
const std::string SGX_TCB_COMP05_SVN = TCB + ".5";
const std::string SGX_TCB_COMP06_SVN = TCB + ".6";
const std::string SGX_TCB_COMP07_SVN = TCB + ".7";
const std::string SGX_TCB_COMP08_SVN = TCB + ".8";
const std::string SGX_TCB_COMP09_SVN = TCB + ".9";
const std::string SGX_TCB_COMP10_SVN = TCB + ".10";
const std::string SGX_TCB_COMP11_SVN = TCB + ".11";
const std::string SGX_TCB_COMP12_SVN = TCB + ".12";
const std::string SGX_TCB_COMP13_SVN = TCB + ".13";
const std::string SGX_TCB_COMP14_SVN = TCB + ".14";
const std::string SGX_TCB_COMP15_SVN = TCB + ".15";
const std::string SGX_TCB_COMP16_SVN = TCB + ".16";
const std::string PCESVN = TCB + ".17";
const std::string CPUSVN = TCB + ".18";
const std::string PCEID = SGX_EXTENSION + ".3";
const std::string FMSPC = SGX_EXTENSION + ".4";
const std::string SGX_TYPE = SGX_EXTENSION + ".5"; // ASN1 Enumerated
const std::string PLATFORM_INSTANCE_ID = SGX_EXTENSION + ".6";
const std::string CONFIGURATION = SGX_EXTENSION + ".7";
const std::string DYNAMIC_PLATFORM = CONFIGURATION + ".1";
const std::string CACHED_KEYS = CONFIGURATION + ".2";
const std::string SMT_ENABLED = CONFIGURATION + ".3";

static const std::map<x509::Extension::Type, std::string> oidEnumToDescription = {
        {x509::Extension::Type::NONE,                 "NONE"},
        {x509::Extension::Type::PPID,                 "PPID"},
        {x509::Extension::Type::CPUSVN,               "CPUSVN"},
        {x509::Extension::Type::PCESVN,               "PCESVN"},
        {x509::Extension::Type::PCEID,                "PCEID"},
        {x509::Extension::Type::FMSPC,                "FMSPC"},
        {x509::Extension::Type::SGX_TYPE,             "SGX_TYPE"},
        {x509::Extension::Type::TCB,                  "TCB"},
        {x509::Extension::Type::SGX_TCB_COMP01_SVN,   "SGX_TCB_COMP01_SVN"},
        {x509::Extension::Type::SGX_TCB_COMP02_SVN,   "SGX_TCB_COMP02_SVN"},
        {x509::Extension::Type::SGX_TCB_COMP03_SVN,   "SGX_TCB_COMP03_SVN"},
        {x509::Extension::Type::SGX_TCB_COMP04_SVN,   "SGX_TCB_COMP04_SVN"},
        {x509::Extension::Type::SGX_TCB_COMP05_SVN,   "SGX_TCB_COMP05_SVN"},
        {x509::Extension::Type::SGX_TCB_COMP06_SVN,   "SGX_TCB_COMP06_SVN"},
        {x509::Extension::Type::SGX_TCB_COMP07_SVN,   "SGX_TCB_COMP07_SVN"},
        {x509::Extension::Type::SGX_TCB_COMP08_SVN,   "SGX_TCB_COMP08_SVN"},
        {x509::Extension::Type::SGX_TCB_COMP09_SVN,   "SGX_TCB_COMP09_SVN"},
        {x509::Extension::Type::SGX_TCB_COMP10_SVN,   "SGX_TCB_COMP10_SVN"},
        {x509::Extension::Type::SGX_TCB_COMP11_SVN,   "SGX_TCB_COMP11_SVN"},
        {x509::Extension::Type::SGX_TCB_COMP12_SVN,   "SGX_TCB_COMP12_SVN"},
        {x509::Extension::Type::SGX_TCB_COMP13_SVN,   "SGX_TCB_COMP13_SVN"},
        {x509::Extension::Type::SGX_TCB_COMP14_SVN,   "SGX_TCB_COMP14_SVN"},
        {x509::Extension::Type::SGX_TCB_COMP15_SVN,   "SGX_TCB_COMP15_SVN"},
        {x509::Extension::Type::SGX_TCB_COMP16_SVN,   "SGX_TCB_COMP16_SVN"},
        {x509::Extension::Type::PLATFORM_INSTANCE_ID, "PLATFORM_INSTANCE_ID"},
        {x509::Extension::Type::CONFIGURATION,        "CONFIGURATION"},
        {x509::Extension::Type::DYNAMIC_PLATFORM,     "DYNAMIC_PLATFORM"},
        {x509::Extension::Type::CACHED_KEYS,          "CACHED_KEYS"},
        {x509::Extension::Type::SMT_ENABLED,          "SMT_ENABLED"}
};

static const std::map<int, std::string> extensionToDescription = {
        {NID_subject_key_identifier,   LN_authority_key_identifier},
        {NID_key_usage,                LN_key_usage},
        {NID_basic_constraints,        LN_basic_constraints},
        {NID_authority_key_identifier, LN_authority_key_identifier},
        {NID_crl_distribution_points,  LN_crl_distribution_points}
};

inline std::string type2Description(const x509::Extension::Type& type)
{
    return oidEnumToDescription.at(type);
}

inline std::string extension2Description(int extension)
{
    return extensionToDescription.at(extension);
}

}; //namespace oids

}}}} // namespace intel { namespace sgx { namespace dcap { namespace pckparser {

#endif //SGX_DCAP_PARSERS_PARSER_UTILS_H
