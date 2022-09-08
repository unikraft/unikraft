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

#include "SgxEcdsaAttestation/AttestationParsers.h"

#include "ParserUtils.h"
#include "OpensslHelpers/OpensslTypes.h"
#include "Utils/TimeUtils.h"

#include <openssl/objects.h>
#include <openssl/x509.h>
#include <openssl/err.h>

#include <algorithm>
#include <chrono>
#include <memory>
#include <sstream>
#include <iomanip>

namespace intel { namespace sgx { namespace dcap { namespace parser {

std::string obj2Str(const ASN1_OBJECT* obj)
{
    if(!obj)
    {
        return "";
    }

    constexpr size_t size = 1024;
    char extname[size];
    OPENSSL_cleanse(extname, size);
    OBJ_obj2txt(extname, size, obj, 1);

    return std::string(extname);
}

std::vector<uint8_t> bn2Vec(const BIGNUM* bn)
{
    if(!bn)
    {
        return {};
    }

    const int bnLen = BN_num_bytes(bn);
    if(bnLen <= 0)
    {
        return {};
    }

    std::vector<uint8_t> ret(static_cast<size_t>(bnLen));

    BN_bn2bin(bn, ret.data());

    return ret;
}

std::string x509NameToString(const X509_NAME* name)
{
    if(!name)
    {
        return "";
    }

    auto bio = crypto::make_unique(BIO_new(BIO_s_mem()));

    X509_NAME_print_ex(bio.get(), name, 0, XN_FLAG_RFC2253);

    char *dataStart;
    const long nameLength = BIO_get_mem_data(bio.get(), &dataStart);

    if(nameLength <= 0)
    {
        return "";
    }

    // dataStart ptr is not null terminated
    // we need to rely on returned size
    std::string ret;
    std::copy_n(dataStart, nameLength, std::back_inserter(ret));

    return ret;
}

std::string getNameEntry(X509_NAME* name, int nid)
{
    // sad, defensive code

    if(!name)
    {
        return "";
    }

    // all returned pointers here are internal openssl
    // pointers so they must not be freed
    // one exception when buffer returned by argument

    if(X509_NAME_entry_count(name) <= 0)
    {
        return "";
    }

    const int position = X509_NAME_get_index_by_NID(name, nid, -1);

    if(position == -1)
    {
        return "";
    }

    X509_NAME_ENTRY *entry = X509_NAME_get_entry(name, position);

    if(!entry)
    {
        return "";
    }

    ASN1_STRING *asn1 = X509_NAME_ENTRY_get_data(entry);
    const int asn1EstimatedStrLen = ASN1_STRING_length(asn1);
    if(asn1EstimatedStrLen <= 0)
    {
        return "";
    }


    unsigned char *ptr; // we need to call OPENSSL_free on this
    const int len = ASN1_STRING_to_UTF8(&ptr, asn1);

    if(len < 0)
    {
        return "";
    }

    std::unique_ptr<unsigned char[], decltype(&crypto::freeOPENSSL)> strBuff(ptr, crypto::freeOPENSSL);

    // shouldn't happened, but who knows
    if(asn1EstimatedStrLen != len)
    {
        return "";
    }


    std::string ret;
    const auto staticCaster = [](unsigned char chr){ return static_cast<char>(chr); };
    std::transform(strBuff.get(), strBuff.get() + len, std::back_inserter(ret), staticCaster);

    return ret;
}

// Converts ASN1_TIME to time_t using ASN1_TIME_diff to get number of seconds from 1 Jan 1970
std::time_t asn1TimeToTimet(
        const ASN1_TIME* asn1Time)
{
    static_assert(sizeof(std::time_t) >= sizeof(int64_t), "std::time_t size too small, the dates may overflow");
    static constexpr int64_t SECONDS_IN_A_DAY = 24 * 60 * 60;

    int pday;
    int psec;
    auto from = crypto::make_unique(ASN1_TIME_new());
    time_t resultTime = 0;
    ASN1_TIME_set(from.get(), resultTime);
    if(1 != ASN1_TIME_diff(&pday, &psec, from.get(), asn1Time))
    {
        // We're here if the format is invalid, thus
        // validity settings in cert should be considered invalid
        throw FormatException(getLastError());
    }

    return resultTime + pday * SECONDS_IN_A_DAY + psec;
}

// Converts a pair of ASN1_TIME time points into a struct wrapping standard time points
// Will throw FormatException on error
std::tuple<time_t, time_t> asn1TimePeriodToCTime(const ASN1_TIME* validityBegin, const ASN1_TIME* validityEnd)
{
    const auto notBeforeTime =  asn1TimeToTimet(validityBegin);
    const auto notAfterTime = asn1TimeToTimet(validityEnd);

    return std::tie(notBeforeTime, notAfterTime);
}

std::string getLastError()
{
    constexpr size_t size = 1024;
    char buff[size];
    OPENSSL_cleanse(buff, size);

    const auto code = ERR_get_error();

    ERR_error_string_n(code, buff, size);

    return std::string(buff);
}

}}}} // namespace intel { namespace sgx { namespace dcap { namespace parser

