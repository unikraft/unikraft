/*############################################################################
# Copyright 2017 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
############################################################################*/
/// Definition of de/serialize functionality.
/*! \file */

#ifndef EPID_MEMBER_TINY_MATH_SERIALIZE_H_
#define EPID_MEMBER_TINY_MATH_SERIALIZE_H_
#include <stddef.h>
#include <stdint.h>

/// \cond
typedef struct OctStr32 OctStr32;

typedef struct VeryLargeInt VeryLargeInt;
typedef struct BigNumStr BigNumStr;

typedef struct FqElem FqElem;
typedef struct FqElemStr FqElemStr;

typedef struct FpElem FpElem;
typedef struct FpElemStr FpElemStr;

typedef struct EccPointFq EccPointFq;
typedef struct G1ElemStr G1ElemStr;

typedef struct EccPointFq2 EccPointFq2;
typedef struct G2ElemStr G2ElemStr;

typedef struct Fq12Elem Fq12Elem;
typedef struct Fq12ElemStr Fq12ElemStr;
/// \endcond

#if !defined(UNOPTIMIZED_SERIALIZATION)

/// Serialize or deserailize a sequence of math objects
/*!
Converts layouts between native and portable or between portable and
native of the following types: VeryLargeInt and BigNumStr,
FqElem and FqElemStr, FpElem and FpElemStr, EccPointFq and G1ElemStr,
Fq12Elem and Fq12ElemStr.

If input contain multiple values of supported types all will be converted.

\note The following types are not supported: VeryLargeIntProduct.

\note This function have the assumptions that the input structures are packed
such that consequent 32 byte fields would have no gap in between.

\param[out] dest target buffer
\param [in] dest_size size of dest buffer
\param [in] src source data
\param [in] src_size size of src buffer
*/
void SwapNativeAndPortableLayout(void* dest, size_t dest_size, void const* src,
                                 size_t src_size);
#endif  // !defined(UNOPTIMIZED_SERIALIZATION)

/// Write a uint32_t to a portable buffer
/*!
\param[out] dest target buffer
\param [in] src source data
\returns pointer to next byte after final data written to dest
*/
void* Uint32Serialize(OctStr32* dest, uint32_t src);

/// Read a uint32_t from a portable buffer
/*!
\param[out] dest target buffer
\param [in] src source data
\returns pointer to next byte after final data read from to src
*/
void const* Uint32Deserialize(uint32_t* dest, OctStr32 const* src);

/// Write a large integer to a portable buffer
/*!
\param[out] dest target buffer
\param [in] src source data
\returns pointer to next byte after final data written to dest
*/
void* VliSerialize(BigNumStr* dest, VeryLargeInt const* src);

/// Read a large integer from a portable buffer
/*!
\param[out] dest target buffer
\param [in] src source data
\returns pointer to next byte after final data read from to src
*/
void const* VliDeserialize(VeryLargeInt* dest, BigNumStr const* src);

/// Write an element of Fq to a portable buffer
/*!
\param[out] dest target buffer
\param [in] src source data
\returns pointer to next byte after final data written to dest
*/
void* FqSerialize(FqElemStr* dest, FqElem const* src);

/// Read an element of Fq from a portable buffer
/*!
\param[out] dest target buffer
\param [in] src source data
\returns pointer to next byte after final data read from to src
*/
void const* FqDeserialize(FqElem* dest, FqElemStr const* src);

/// Write an element of Fq12 to a portable buffer
/*!
\param[out] dest target buffer
\param [in] src source data
\returns pointer to next byte after final data written to dest
*/
void* Fq12Serialize(Fq12ElemStr* dest, Fq12Elem const* src);

/// Read an element of Fq12 from a portable buffer
/*!
\param[out] dest target buffer
\param [in] src source data
\returns pointer to next byte after final data read from to src
*/
void const* Fq12Deserialize(Fq12Elem* dest, Fq12ElemStr const* src);

/// Write an element of Fp to a portable buffer
/*!
\param[out] dest target buffer
\param [in] src source data
\returns pointer to next byte after final data written to dest
*/
void* FpSerialize(FpElemStr* dest, FpElem const* src);

/// Read an element of Fp from a portable buffer
/*!
\param[out] dest target buffer
\param [in] src source data
\returns pointer to next byte after final data read from to src
*/
void const* FpDeserialize(FpElem* dest, FpElemStr const* src);

/// Write a point on EFq to a portable buffer
/*!
\param[out] dest target buffer
\param [in] src source data
\returns pointer to next byte after final data written to dest
*/
void* EFqSerialize(G1ElemStr* dest, EccPointFq const* src);

/// Read a point on EFq from a portable buffer
/*!
\param[out] dest target buffer
\param [in] src source data
\returns pointer to next byte after final data read from to src
*/
void const* EFqDeserialize(EccPointFq* dest, G1ElemStr const* src);

/// Write a point on EFq2 to a portable buffer
/*!
\param[out] dest target buffer
\param [in] src source data
\returns pointer to next byte after final data written to dest
*/
void* EFq2Serialize(G2ElemStr* dest, EccPointFq2 const* src);

/// Read a point on EFq2 from a portable buffer
/*!
\param[out] dest target buffer
\param [in] src source data
\returns pointer to next byte after final data read from to src
*/
void const* EFq2Deserialize(EccPointFq2* dest, G2ElemStr const* src);

#endif  // EPID_MEMBER_TINY_MATH_SERIALIZE_H_
