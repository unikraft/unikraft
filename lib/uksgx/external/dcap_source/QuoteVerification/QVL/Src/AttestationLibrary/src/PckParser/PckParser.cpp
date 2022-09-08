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

#include "PckParser.h"

#include <cstring>
#include <algorithm>
#include <iterator>
#include <map>
#include <iomanip>
#include <Utils/TimeUtils.h>
#include <Utils/SafeMemcpy.h>

#include "FormatException.h"
#include "PckParserUtils.h"

namespace intel { namespace sgx { namespace dcap { namespace pckparser {

bool Extension::operator==(const Extension& other) const
{
    return
        opensslNid == other.opensslNid
        && name == other.name
        && value == other.value;
}

bool Extension::operator!=(const Extension& other) const
{
    return !(*this == other);
}

bool Issuer::operator==(const Issuer& other) const
{
    return commonName == other.commonName
        && countryName == other.countryName
        && locationName == other.locationName
        && stateName == other.stateName
        && organizationName == other.organizationName;
}

bool Issuer::operator!=(const Issuer& other) const
{
    return !(*this == other);
}

bool Issuer::operator==(const Subject& subject) const
{
    return commonName == subject.commonName
        && countryName == subject.countryName
        && locationName == subject.locationName
        && stateName == subject.stateName
        && organizationName == subject.organizationName;
}

bool Issuer::operator!=(const Subject& subject) const
{
    return !(*this == subject);
}

bool Subject::operator==(const Subject& other) const
{
    return commonName == other.commonName
        && countryName == other.countryName
        && locationName == other.locationName
        && stateName == other.stateName
        && organizationName == other.organizationName;
}

bool Revoked::operator==(const Revoked& other) const
{
    return serialNumber == other.serialNumber;
}

bool Revoked::operator!=(const Revoked& other) const
{
    return !(*this == other);
}

bool Subject::operator!=(const Subject& other) const
{
    return !(*this == other);
}

bool Subject::operator==(const Issuer& issuer) const
{
    return commonName == issuer.commonName
        && countryName == issuer.countryName
        && locationName == issuer.locationName
        && stateName == issuer.stateName
        && organizationName == issuer.organizationName;
}

bool Subject::operator!=(const Issuer& issuer) const
{
    return !(*this == issuer);
}

bool Validity::isValid(const time_t& expirationDate) const
{
    return expirationDate <= notAfterTime && expirationDate >= notBeforeTime;
}

bool Validity::operator==(const Validity& other) const
{
    return notBeforeTime == other.notBeforeTime
           && notAfterTime == other.notAfterTime;
}

namespace { // anonymous namespace

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

Signature getSignature(const ASN1_BIT_STRING* psig)
{
    std::vector<uint8_t> rawDerSequence(static_cast<size_t>(psig->length));
    safeMemcpy(rawDerSequence.data(), psig->data, static_cast<size_t>(psig->length));

    const uint8_t *derSeqIt = rawDerSequence.data();
    const auto sig = crypto::make_unique(d2i_ECDSA_SIG(nullptr, &derSeqIt , static_cast<long>(rawDerSequence.size())));

    if(!sig)
    {
        return Signature{
            rawDerSequence,
            {},
            {}
        };
    }

    // internal pointers
    const BIGNUM *r,*s;
    ECDSA_SIG_get0(sig.get(), &r, &s);

    return Signature{
        rawDerSequence,
        bn2Vec(r),
        bn2Vec(s)
    };
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

std::string asn1ToString(const ASN1_TIME* time)
{
    if(!time)
    {
        return "";
    }

    constexpr size_t size = 256;
    auto bio = crypto::make_unique(BIO_new(BIO_s_mem()));
    int rc = ASN1_TIME_print(bio.get(), time);
    if(rc <= 0)
    {
        return "";
    }

    char buff[size];
    OPENSSL_cleanse(buff, size);

    rc = BIO_gets(bio.get(), buff, size);
    if(rc <= 0)
    {
        return "";
    }

    return std::string(buff);
}

Extension getExtension(X509_EXTENSION* ex)
{
    // I do not check for nullptr below cause this func is not part of
    // public API and given a place from where call is perfmormed I'm pretty certain
    // there won't be any here
    const ASN1_OBJECT *asn1Obj = X509_EXTENSION_get_object(ex);

    const int nid = OBJ_obj2nid(asn1Obj);

    if(nid == NID_undef)
    { // we encountered our custom entry
      // name handling is diffrent and its value is
      // equal to one of our custom OID

        const std::string extName = obj2Str(asn1Obj);

        const auto val = X509_EXTENSION_get_data(ex);

        if(!val)
        {
            throw FormatException("Invalid Extension");
        }

        std::vector<uint8_t> data(static_cast<size_t>(val->length));
        std::copy_n(val->data, val->length, data.begin());

        return Extension {
            nid,
            extName,
            data
        };
    }

    auto bio = crypto::make_unique(BIO_new(BIO_s_mem()));
    if(!X509V3_EXT_print(bio.get(), ex, 0, 0))
    {   // revocation extensions may not be printable,
        // at the time of writing this code, there was no revocation
        // extensions planned so this 'if' statement probably won't be
        // hit at all
        const auto val = X509_EXTENSION_get_data(ex);

        std::vector<uint8_t> data(static_cast<size_t>(val->length));
        std::copy_n(val->data, val->length, data.begin());

        return Extension{
            nid,
            std::string(OBJ_nid2ln(nid)),
            data
        };
    }

    BUF_MEM *bptr; // this will be freed when bio will be closed
    BIO_get_mem_ptr(bio.get(), &bptr);

    std::vector<uint8_t> data(static_cast<size_t>(bptr->length));

    const auto noNewLineChar = [](uint8_t chr){ return chr != '\r' && chr != '\n'; };
    std::copy_if(bptr->data, bptr->data + bptr->length, data.begin(), noNewLineChar);

    return Extension{
        nid,
        std::string(OBJ_nid2ln(nid)),
        data
    };
}

Revoked getRevoked(const STACK_OF(X509_REVOKED)* revokedStack, int index)
{
    // internal pointer
    const X509_REVOKED *revoked = sk_X509_REVOKED_value(revokedStack, index);

    if(!revoked)
    {
        return {};
    }

    // internal pointers
    const ASN1_INTEGER *serialNumber = X509_REVOKED_get0_serialNumber(revoked);
    const ASN1_TIME *time = X509_REVOKED_get0_revocationDate(revoked);

    if(!serialNumber || !time)
    {
        return {};
    }

    if(serialNumber->length <= 0)
    {
        return {};
    }

    std::vector<uint8_t> retSerial(static_cast<size_t>(serialNumber->length));
    std::copy_n(serialNumber->data, serialNumber->length, retSerial.begin());

    // Revoked entry can have its extensions
    // but at the time of writing this code there was no
    // revoked extensions defined in spec so I just leave this
    // as a note.
    // const STACK_OF(X509_EXTENSION) *revokedExtensions = X509_REVOKED_get0_extensions(revoked);

    return Revoked{
        asn1ToString(time),
        retSerial
    };
}

bool initialized = false;

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
Validity asn1TimePeriodToValidity(const ASN1_TIME* validityBegin, const ASN1_TIME* validityEnd)
{
    const auto notBeforeTime = asn1TimeToTimet(validityBegin);
    const auto notAfterTime = asn1TimeToTimet(validityEnd);

    return Validity{notBeforeTime, notAfterTime};
}

} // anonymous namespace

void initOpenSSL()
{
    if(initialized) return;

    // To use X509 function group
    OpenSSL_add_all_algorithms();

    // For error strings
    ERR_load_crypto_strings();
}

void cleanUpOpenSSL()
{
    if(initialized)
    {
        ERR_free_strings();
    }
}

crypto::X509_CRL_uptr str2X509Crl(const std::string& string)
{
    auto bio_mem = crypto::make_unique(BIO_new(BIO_s_mem()));
    crypto::X509_CRL_uptr ret = crypto::make_unique<X509_CRL>(nullptr);
    if(string.rfind(PEM_STRING_X509_CRL, 12) == std::string::npos)
    {
        // Attempt to read CRL as DER
        const auto bytes = hexStringToBytes(string);
        const auto ec = BIO_write(bio_mem.get(), bytes.data(), (int) bytes.size());
        if (ec < 1)
        {
            throw FormatException(getLastError());
        }
        ret.reset(d2i_X509_CRL_bio(bio_mem.get(), nullptr));
    }
    else
    {
        // Attempt to read CRL as PEM
        const auto ec = BIO_puts(bio_mem.get(), string.c_str());
        if (ec < 1)
        {
            throw FormatException(getLastError());
        }
        ret.reset(PEM_read_bio_X509_CRL(bio_mem.get(), nullptr, nullptr, nullptr));
    }

    if(!ret)
    {
        throw FormatException(getLastError());
    }

    return ret;
}

long getVersion(const X509_CRL& crl)
{
    // version is zero-indexed thus +1
    return X509_CRL_get_version(&crl) + 1;
}

Issuer getIssuer(const X509_CRL& crl)
{
    X509_NAME *issuer = X509_CRL_get_issuer(&crl);
    if(!issuer)
    {
        throw FormatException(getLastError());
    }

    return Issuer{
        x509NameToString(issuer),
        getNameEntry(issuer, NID_commonName),
        getNameEntry(issuer, NID_countryName),
        getNameEntry(issuer, NID_organizationName),
        getNameEntry(issuer, NID_localityName),
        getNameEntry(issuer, NID_stateOrProvinceName)
    };
}

int getExtensionCount(const X509_CRL& crl)
{
    const int extsCount = X509_CRL_get_ext_count(&crl);

    if(extsCount < 0)
    {
        throw FormatException(getLastError());
    }

    return extsCount;
}

std::vector<Extension> getExtensions(const X509_CRL& crl)
{
    const int extsCount = X509_CRL_get_ext_count(&crl);

    if(extsCount == 0)
    {
        return {};
    }

    std::vector<Extension> ret(static_cast<size_t>(extsCount));
    int index = 0;
    std::generate(ret.begin(), ret.end(), [&crl, &index]{return getExtension(X509_CRL_get_ext(&crl, index++));});

    return ret;
}

Signature getSignature(const X509_CRL& crl)
{
    // both internal pointers and must not be freed
    const ASN1_BIT_STRING *psig = nullptr;
    const X509_ALGOR *palg = nullptr;
    X509_CRL_get0_signature(&crl, &psig, &palg);

    if(!psig || !palg)
    {
        throw FormatException(getLastError());
    }

    if(psig->length == 0)
    {
        return {};
    }

    return getSignature(psig);
}

int getRevokedCount(X509_CRL& crl)
{
    // this is internal pointer, must not be freed
    const STACK_OF(X509_REVOKED) *revokedStack = X509_CRL_get_REVOKED(&crl);

    if(!revokedStack)
    {
        // as test shown, stack is not initialized when no
        // revoked elements present, thus it's necessarily indication
        // of bad format
        return 0;
    }

    const int ret = sk_X509_REVOKED_num(revokedStack);

    if(ret < 0)
    {
        throw FormatException(getLastError());
    }

    return ret;
}

std::vector<Revoked> getRevoked(X509_CRL& crl)
{
    // this is internal pointer, must not be freed
    const STACK_OF(X509_REVOKED) *revokedStack = X509_CRL_get_REVOKED(&crl);

    if(!revokedStack)
    {
        // as test shown, stack is not initialized when no
        // revoked elements present, thus it's necessarily indication
        // of bad format
        return {};
    }

    const int revCount = getRevokedCount(crl);

    if(revCount == 0)
    {
        return {};
    }

    std::vector<Revoked> ret(static_cast<size_t>(revCount));
    int index = 0;
    std::generate(ret.begin(), ret.end(), [&revokedStack, &index]{ return getRevoked(revokedStack, index++);});
    return ret;
}

Validity getValidity(const X509_CRL& crl)
{
    const ASN1_TIME *lastUpdate = X509_CRL_get0_lastUpdate(&crl);
    if(!lastUpdate)
    {
        throw FormatException(getLastError());
    }

    const ASN1_TIME *nextUpdate = X509_CRL_get0_nextUpdate(&crl);
    if(!nextUpdate)
    {
        throw FormatException(getLastError());
    }

    return asn1TimePeriodToValidity(lastUpdate, nextUpdate);
}

long getCrlNum(X509_CRL& crl)
{
    crypto::ASN1_INTEGER_uptr crlNum =
            crypto::make_unique(
                    static_cast<ASN1_INTEGER *>(X509_CRL_get_ext_d2i(&crl, NID_crl_number, nullptr, nullptr)));

    if(!crlNum)
    {
        throw FormatException(getLastError());
    }

    return ASN1_INTEGER_get(crlNum.get());
}

}}}} // namespace intel { namespace sgx { namespace dcap { namespace pckparser {
