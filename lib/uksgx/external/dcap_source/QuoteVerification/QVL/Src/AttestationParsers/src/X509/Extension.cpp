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

#include "ParserUtils.h" // obj2Str
#include "OpensslHelpers/OpensslTypes.h"

#include <algorithm>

namespace intel { namespace sgx { namespace dcap { namespace parser { namespace x509 {

Extension::Extension(): _nid{},
                        _name{},
                        _value{}
{}

Extension::Extension(int nid,
                     const std::string& name,
                     const std::vector<uint8_t>& value) noexcept:
                        _nid(nid),
                        _name(name),
                        _value(value)
{}

int Extension::getNid() const
{
    return _nid;
}

const std::string& Extension::getName() const
{
    return _name;
}

const std::vector<uint8_t>& Extension::getValue() const
{
    return _value;
}

bool Extension::operator==(const Extension& other) const
{
    return _nid == other._nid &&
           _name == other._name &&
           _value == other._value;
}

bool Extension::operator!=(const Extension& other) const
{
    return !operator==(other);
}

// Private

Extension::Extension(X509_EXTENSION *ext)
{
    // I do not check for nullptr below cause this func is not part of
    // public API and given a place from where call is performed I'm pretty certain
    // there won't be any here
    const ASN1_OBJECT *asn1Obj = X509_EXTENSION_get_object(ext);

    _nid = OBJ_obj2nid(asn1Obj);

    if(_nid == NID_undef)
    {   // we encountered our custom entry
        // name handling is different and its value is
        // equal to one of our custom OID

        _name = obj2Str(asn1Obj);

        const auto val = X509_EXTENSION_get_data(ext);

        if(!val)
        {
            throw FormatException("Invalid Extension");
        }

        _value = std::vector<uint8_t>(static_cast<size_t>(val->length));
        std::copy_n(val->data, val->length, _value.begin());

        return;
    }

    auto bio = crypto::make_unique(BIO_new(BIO_s_mem()));
    if(!X509V3_EXT_print(bio.get(), ext, 0, 0))
    {   // revocation extensions may not be printable,
        // at the time of writing this code, there was no revocation
        // extensions planned so this 'if' statement probably won't be
        // hit at all
        const auto val = X509_EXTENSION_get_data(ext);

        _value = std::vector<uint8_t>(static_cast<size_t>(val->length));
        std::copy_n(val->data, val->length, _value.begin());

        _name = std::string(OBJ_nid2ln(_nid));

        return;
    }

    BUF_MEM *bptr; // this will be freed when bio will be closed
    BIO_get_mem_ptr(bio.get(), &bptr);

    _value = std::vector<uint8_t>(static_cast<size_t>(bptr->length));

    const auto noNewLineChar = [](uint8_t chr){ return chr != '\r' && chr != '\n'; };
    std::copy_if(bptr->data, bptr->data + bptr->length, _value.begin(), noNewLineChar);

    _name = std::string(OBJ_nid2ln(_nid));
}

}}}}} // namespace intel { namespace sgx { namespace dcap { namespace parser { namespace x509 {
