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

#ifndef SGX_DCAP_PARSERS_TEST_X509_CERT_TYPES_H
#define SGX_DCAP_PARSERS_TEST_X509_CERT_TYPES_H

#include "OpensslHelpers/OpensslTypes.h"
#include <openssl/asn1.h>
#include <openssl/asn1t.h>

namespace intel { namespace sgx { namespace dcap { namespace crypto {

typedef struct SGX_INT_st {
    ASN1_OBJECT *oid;
    ASN1_INTEGER *value;
} SGX_INT;

ASN1_SEQUENCE(SGX_INT) = {
        ASN1_SIMPLE(SGX_INT, oid, ASN1_OBJECT),
        ASN1_SIMPLE(SGX_INT, value, ASN1_INTEGER)
} ASN1_SEQUENCE_END(SGX_INT)

DECLARE_ASN1_FUNCTIONS(SGX_INT)

typedef struct SGX_ENUM_st {
    ASN1_OBJECT *oid;
    ASN1_ENUMERATED *value;
} SGX_ENUM;

ASN1_SEQUENCE(SGX_ENUM) = {
        ASN1_SIMPLE(SGX_ENUM, oid, ASN1_OBJECT),
        ASN1_SIMPLE(SGX_ENUM, value, ASN1_ENUMERATED)
} ASN1_SEQUENCE_END(SGX_ENUM)

DECLARE_ASN1_FUNCTIONS(SGX_ENUM)

typedef struct SGX_BOOL_st {
    ASN1_OBJECT *oid;
    ASN1_BOOLEAN *value;
} SGX_BOOL;

ASN1_SEQUENCE(SGX_BOOL) = {
        ASN1_SIMPLE(SGX_BOOL, oid, ASN1_OBJECT),
        ASN1_SIMPLE(SGX_BOOL, value, ASN1_BOOLEAN)
} ASN1_SEQUENCE_END(SGX_BOOL)

DECLARE_ASN1_FUNCTIONS(SGX_BOOL)

typedef struct SGX_OCTET_STRING_st {
    ASN1_OBJECT *oid;
    ASN1_OCTET_STRING *value;
} SGX_OCTET_STRING;

ASN1_SEQUENCE(SGX_OCTET_STRING) = {
        ASN1_SIMPLE(SGX_OCTET_STRING, oid, ASN1_OBJECT),
        ASN1_SIMPLE(SGX_OCTET_STRING, value, ASN1_OCTET_STRING)
} ASN1_SEQUENCE_END(SGX_OCTET_STRING)

DECLARE_ASN1_FUNCTIONS(SGX_OCTET_STRING)

typedef struct SGX_TCB_st {
    SGX_INT *comp01;
    SGX_INT *comp02;
    SGX_INT *comp03;
    SGX_INT *comp04;
    SGX_INT *comp05;
    SGX_INT *comp06;
    SGX_INT *comp07;
    SGX_INT *comp08;
    SGX_INT *comp09;
    SGX_INT *comp10;
    SGX_INT *comp11;
    SGX_INT *comp12;
    SGX_INT *comp13;
    SGX_INT *comp14;
    SGX_INT *comp15;
    SGX_INT *comp16;
    SGX_INT *pcesvn;
    SGX_OCTET_STRING *cpusvn;
} SGX_TCB;

ASN1_SEQUENCE(SGX_TCB) = {
        ASN1_SIMPLE(SGX_TCB, comp01, SGX_INT),
        ASN1_SIMPLE(SGX_TCB, comp02, SGX_INT),
        ASN1_SIMPLE(SGX_TCB, comp03, SGX_INT),
        ASN1_SIMPLE(SGX_TCB, comp04, SGX_INT),
        ASN1_SIMPLE(SGX_TCB, comp05, SGX_INT),
        ASN1_SIMPLE(SGX_TCB, comp06, SGX_INT),
        ASN1_SIMPLE(SGX_TCB, comp07, SGX_INT),
        ASN1_SIMPLE(SGX_TCB, comp08, SGX_INT),
        ASN1_SIMPLE(SGX_TCB, comp09, SGX_INT),
        ASN1_SIMPLE(SGX_TCB, comp10, SGX_INT),
        ASN1_SIMPLE(SGX_TCB, comp11, SGX_INT),
        ASN1_SIMPLE(SGX_TCB, comp12, SGX_INT),
        ASN1_SIMPLE(SGX_TCB, comp13, SGX_INT),
        ASN1_SIMPLE(SGX_TCB, comp14, SGX_INT),
        ASN1_SIMPLE(SGX_TCB, comp15, SGX_INT),
        ASN1_SIMPLE(SGX_TCB, comp16, SGX_INT),
        ASN1_SIMPLE(SGX_TCB, pcesvn, SGX_INT),
        ASN1_SIMPLE(SGX_TCB, cpusvn, SGX_OCTET_STRING)
} ASN1_SEQUENCE_END(SGX_TCB)

DECLARE_ASN1_FUNCTIONS(SGX_TCB)

typedef struct SGX_TCB_SEQ_st {
    ASN1_OBJECT *oid;
    SGX_TCB *tcb;
} SGX_TCB_SEQ;

ASN1_SEQUENCE(SGX_TCB_SEQ) = {
        ASN1_SIMPLE(SGX_TCB_SEQ, oid, ASN1_OBJECT),
        ASN1_SIMPLE(SGX_TCB_SEQ, tcb, SGX_TCB)
} ASN1_SEQUENCE_END(SGX_TCB_SEQ)

DECLARE_ASN1_FUNCTIONS(SGX_TCB_SEQ)

typedef struct SGX_CONFIGURATION_st {
    SGX_BOOL *dynamicPlatform;
    SGX_BOOL *cachedKeys;
    SGX_BOOL *smtEnabled;
} SGX_CONFIGURATION;

ASN1_SEQUENCE(SGX_CONFIGURATION) = {
        ASN1_OPT(SGX_CONFIGURATION, dynamicPlatform, SGX_BOOL),
        ASN1_OPT(SGX_CONFIGURATION, cachedKeys, SGX_BOOL),
        ASN1_OPT(SGX_CONFIGURATION, smtEnabled, SGX_BOOL)
} ASN1_SEQUENCE_END(SGX_CONFIGURATION)

DECLARE_ASN1_FUNCTIONS(SGX_CONFIGURATION)

typedef struct SGX_CONFIGURATION_SEQ_st {
    ASN1_OBJECT *oid;
    SGX_CONFIGURATION *configuration;
} SGX_CONFIGURATION_SEQ;

ASN1_SEQUENCE(SGX_CONFIGURATION_SEQ) = {
        ASN1_SIMPLE(SGX_CONFIGURATION_SEQ, oid, ASN1_OBJECT),
        ASN1_SIMPLE(SGX_CONFIGURATION_SEQ, configuration, SGX_CONFIGURATION)
} ASN1_SEQUENCE_END(SGX_CONFIGURATION_SEQ)

DECLARE_ASN1_FUNCTIONS(SGX_CONFIGURATION_SEQ)

typedef struct SGX_EXTENSIONS_st {
    SGX_OCTET_STRING *ppid;
    SGX_TCB_SEQ *tcb;
    SGX_OCTET_STRING *pceid;
    SGX_OCTET_STRING *fmspc;
    SGX_ENUM *sgxType;
    SGX_OCTET_STRING *platformInstanceId;
    SGX_CONFIGURATION_SEQ *configuration;
    SGX_BOOL *unexpectedExtension;
} SGX_EXTENSIONS;

ASN1_SEQUENCE(SGX_EXTENSIONS) = {
        ASN1_SIMPLE(SGX_EXTENSIONS, ppid, SGX_OCTET_STRING),
        ASN1_SIMPLE(SGX_EXTENSIONS, tcb, SGX_TCB_SEQ),
        ASN1_SIMPLE(SGX_EXTENSIONS, pceid, SGX_OCTET_STRING),
        ASN1_SIMPLE(SGX_EXTENSIONS, fmspc, SGX_OCTET_STRING),
        ASN1_SIMPLE(SGX_EXTENSIONS, sgxType, SGX_ENUM),
        ASN1_OPT(SGX_EXTENSIONS, platformInstanceId, SGX_OCTET_STRING),
        ASN1_OPT(SGX_EXTENSIONS, configuration, SGX_CONFIGURATION_SEQ),
        ASN1_OPT(SGX_EXTENSIONS, unexpectedExtension, SGX_BOOL)
} ASN1_SEQUENCE_END(SGX_EXTENSIONS)

DECLARE_ASN1_FUNCTIONS(SGX_EXTENSIONS)

using SGX_INT_uptr               = std::unique_ptr<SGX_INT,               decltype(&SGX_INT_free)>;
using SGX_ENUM_uptr              = std::unique_ptr<SGX_ENUM,              decltype(&SGX_ENUM_free)>;
using SGX_BOOL_uptr              = std::unique_ptr<SGX_BOOL,              decltype(&SGX_BOOL_free)>;
using SGX_OCTET_STRING_uptr      = std::unique_ptr<SGX_OCTET_STRING,      decltype(&SGX_OCTET_STRING_free)>;
using SGX_TCB_uptr               = std::unique_ptr<SGX_TCB,               decltype(&SGX_TCB_free)>;
using SGX_TCB_SEQ_uptr           = std::unique_ptr<SGX_TCB_SEQ,           decltype(&SGX_TCB_SEQ_free)>;
using SGX_EXTENSIONS_uptr        = std::unique_ptr<SGX_EXTENSIONS,        decltype(&SGX_EXTENSIONS_free)>;
using SGX_CONFIGURATION_uptr     = std::unique_ptr<SGX_CONFIGURATION,     decltype(&SGX_CONFIGURATION_free)>;
using SGX_CONFIGURATION_SEQ_uptr = std::unique_ptr<SGX_CONFIGURATION_SEQ, decltype(&SGX_CONFIGURATION_SEQ_free)>;
using ASN1_OBJECT_uptr           = std::unique_ptr<ASN1_OBJECT,           decltype(&ASN1_OBJECT_free)>;
/* Base template for the making of OpenSSL crypto objects, automatically selected deleter - unique_ptr variant */
//template<typename T>
//inline auto make_unique(T*) -> std::unique_ptr<T, void(*)(T*)>;

template<>
inline SGX_INT_uptr make_unique(SGX_INT* raw_pointer)
{
    return SGX_INT_uptr(raw_pointer, SGX_INT_free);
}

template<>
inline SGX_ENUM_uptr make_unique(SGX_ENUM* raw_pointer)
{
    return SGX_ENUM_uptr(raw_pointer, SGX_ENUM_free);
}

template<>
inline SGX_BOOL_uptr make_unique(SGX_BOOL* raw_pointer)
{
    return SGX_BOOL_uptr(raw_pointer, SGX_BOOL_free);
}

template<>
inline SGX_OCTET_STRING_uptr make_unique(SGX_OCTET_STRING* raw_pointer)
{
    return SGX_OCTET_STRING_uptr(raw_pointer, SGX_OCTET_STRING_free);
}

template<>
inline SGX_TCB_uptr make_unique(SGX_TCB* raw_pointer)
{
    return SGX_TCB_uptr(raw_pointer, SGX_TCB_free);
}

template<>
inline SGX_TCB_SEQ_uptr make_unique(SGX_TCB_SEQ* raw_pointer)
{
    return SGX_TCB_SEQ_uptr(raw_pointer, SGX_TCB_SEQ_free);
}

template<>
inline SGX_EXTENSIONS_uptr make_unique(SGX_EXTENSIONS* raw_pointer)
{
    return SGX_EXTENSIONS_uptr(raw_pointer, SGX_EXTENSIONS_free);
}

template<>
inline SGX_CONFIGURATION_uptr make_unique(SGX_CONFIGURATION* raw_pointer)
{
    return SGX_CONFIGURATION_uptr(raw_pointer, SGX_CONFIGURATION_free);
}

template<>
inline SGX_CONFIGURATION_SEQ_uptr make_unique(SGX_CONFIGURATION_SEQ* raw_pointer)
{
    return SGX_CONFIGURATION_SEQ_uptr(raw_pointer, SGX_CONFIGURATION_SEQ_free);
}

template<>
inline ASN1_OBJECT_uptr make_unique(ASN1_OBJECT* raw_pointer)
{
    return ASN1_OBJECT_uptr(raw_pointer, ASN1_OBJECT_free);
}

}}}} // namespace intel { namespace sgx { namespace dcap { namespace crypto {

#endif //SGX_DCAP_PARSERS_TEST_X509_CERT_TYPES_H
