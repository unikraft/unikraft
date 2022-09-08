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


#ifndef SGX_INTEL_PCKLIB_PCKPARSER_H_
#define SGX_INTEL_PCKLIB_PCKPARSER_H_

#include <string>
#include <utility>
#include <vector>
#include <sstream>
#include <ctime>
#include <chrono>

#include <OpensslHelpers/OpensslTypes.h>
#include <OpensslHelpers/Bytes.h>
#include <iostream>

using namespace intel::sgx::dcap;

namespace intel { namespace sgx { namespace dcap { namespace pckparser {

struct Subject;
struct Issuer
{
    std::string raw;
    std::string commonName;
    std::string countryName;
    std::string organizationName;
    std::string locationName;
    std::string stateName;

    bool operator ==(const Issuer& other) const;
    bool operator !=(const Issuer& other) const;

    bool operator ==(const Subject& subject) const;
    bool operator !=(const Subject& subject) const;
};

struct Subject
{
    std::string raw;
    std::string commonName;
    std::string countryName;
    std::string organizationName;
    std::string locationName;
    std::string stateName;

    bool operator ==(const Subject& other) const;
    bool operator !=(const Subject& other) const;

    bool operator ==(const Issuer& issuer) const;
    bool operator !=(const Issuer& issuer) const;
};

struct Validity
{
    bool isValid(const time_t& expirationDate) const;
    bool operator ==(const Validity& other) const;

    std::time_t notBeforeTime;
    std::time_t notAfterTime;
};

struct Extension
{
    int opensslNid;
    std::string name;
    std::vector<uint8_t> value;

    bool operator==(const Extension&) const;
    bool operator!=(const Extension&) const;
};

struct Signature
{
    std::vector<uint8_t> rawDer;
    std::vector<uint8_t> r;
    std::vector<uint8_t> s;
};

struct Revoked
{
    std::string dateStr;
    std::vector<uint8_t> serialNumber;
    
    bool operator==(const Revoked&) const;
    bool operator!=(const Revoked&) const; 

    // revoked elements can have their own extensions
    // but at the time of writing this lib no extensions
    // were defined in spec 
    // std::vector<Extension> extensions;
};

// OpenSSL initialization/cleanup functions.
// init* to be called once at the beginning of execution.
// clean* to be called one at the end of execution
// You do not need to call these if you already have OpenSSL initialized
// by different code.
// THESE FUNCTIONS PERFORM MINIMAL INITIALIZATION
void initOpenSSL();
void cleanUpOpenSSL();

////////////////////////////////////////////////////////////////////////////
//
//      PCK Certificate revocation list functions
//
////////////////////////////////////////////////////////////////////////////

crypto::X509_CRL_uptr str2X509Crl(const std::string& data);
long getVersion(const X509_CRL& crl);
Issuer getIssuer(const X509_CRL& crl);
int getExtensionCount(const X509_CRL& crl);
std::vector<Extension> getExtensions(const X509_CRL& crl);
Signature getSignature(const X509_CRL& crl);
Validity getValidity(const X509_CRL& crl);

int getRevokedCount(X509_CRL& crl);
std::vector<Revoked> getRevoked(X509_CRL& crl);
long getCrlNum(X509_CRL& crl);

}}}} // namespace intel { namespace sgx { namespace dcap { namespace pckparser {

#endif // SGX_INTEL_PCKLIB_PCKPARSER_H_
