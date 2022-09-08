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
/// TPM-SDK data conversion interface.
/*! \file */
#ifndef EPID_MEMBER_TPM2_IBM_TSS_CONVERSION_H_
#define EPID_MEMBER_TPM2_IBM_TSS_CONVERSION_H_

#include <tss2/TPM_Types.h>
#include "epid/common/errors.h"
#include "epid/common/types.h"

#ifndef TPM_ALG_SHA256
/// TPM code of SHA 256 algorithm
#define TPM_ALG_SHA256 (TPM_ALG_ID)(0x000B)
#endif
#ifndef TPM_ALG_SHA384
/// TPM code of SHA 384 algorithm
#define TPM_ALG_SHA384 (TPM_ALG_ID)(0x000C)
#endif
#ifndef TPM_ALG_SHA512
/// TPM code of SHA 512 algorithm
#define TPM_ALG_SHA512 (TPM_ALG_ID)(0x000D)
#endif
#ifndef TPM_ALG_NULL
/// TPM code of Null algorithm
#define TPM_ALG_NULL (TPM_ALG_ID)(0x0010)
#endif

/// \cond
typedef struct G1ElemStr G1ElemStr;
/// \endcond

/// Maps HashAlg to TPM type
/*!
Maps Intel(R) EPID SDK HashAlg into TPMI_ALG_HASH.

\param[in] hash_alg
Code of the hash algorithm
\returns TPMI_ALG_HASH
*/
TPMI_ALG_HASH EpidtoTpm2HashAlg(HashAlg hash_alg);

/// Maps TPMI_ALG_HASH to HashAlg
/*!
Maps TPM hash code TPMI_ALG_HASH into HashAlg.

\param[in] tpm_hash_alg
Code of the hash algorithm in TPM

\returns HashAlg
*/
HashAlg Tpm2toEpidHashAlg(TPMI_ALG_HASH tpm_hash_alg);

/// Converts serialized FfElement into TPM type
/*!

\param[in] str
Serialized Intel(R) EPID SDK FfElement
\param[out] tpm_data
tpm type data.
\returns ::EpidStatus
*/
EpidStatus ReadTpm2FfElement(OctStr256 const* str,
                             TPM2B_ECC_PARAMETER* tpm_data);

/// Converts TPM finite field element types into serialized FfElement
/*!

\param[in] tpm_data
The TPM finite field data, typically TPM2B_DIGEST or
TPM2B_ECC_PARAMETER.
\param[out] str
The target buffer.

\returns ::EpidStatus
*/
EpidStatus WriteTpm2FfElement(TPM2B_ECC_PARAMETER const* tpm_data,
                              OctStr256* str);

/// Converts ECPoint string to TMP ECPoint structure.
/*!

 \param[in] p_str
 The serialized EcPoint to convert.
 \param[out] tpm_point
 The TPM EC point representation.

 \returns ::EpidStatus
*/
EpidStatus ReadTpm2EcPoint(G1ElemStr const* p_str, TPM2B_ECC_POINT* tpm_point);

/// Serializes TMP ECPoint to ECPoint string.
/*!

 \param[in] tpm_point
 The TPM EC point to convert.
 \param[in] p_str
 The target string.

\returns ::EpidStatus
*/
EpidStatus WriteTpm2EcPoint(TPM2B_ECC_POINT const* tpm_point, G1ElemStr* p_str);

#endif  // EPID_MEMBER_TPM2_IBM_TSS_CONVERSION_H_
