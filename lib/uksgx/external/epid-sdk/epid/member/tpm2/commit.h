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
/// SDK TPM Commit API.
/*! \file */

#ifndef EPID_MEMBER_TPM2_COMMIT_H_
#define EPID_MEMBER_TPM2_COMMIT_H_

#include <stddef.h>

#include "epid/common/errors.h"
#include "epid/common/stdtypes.h"

/// \cond
typedef struct Tpm2Ctx Tpm2Ctx;
typedef struct FfElement FfElement;
typedef struct EcPoint EcPoint;
/// \endcond

/*!
\addtogroup Tpm2Module tpm2
\ingroup EpidMemberModule
@{
*/

/// Performs TPM2_Commit TPM operation.
/*!
Generates random r and compute K, L and E points.

\param[in] ctx
The TPM context.
\param[in] p1
A point P1 on G1 curve.
\param[in] s2
Octet array used to derive x-coordinate of a point P2.
\param[in] s2_len
Length of s2 buffer.
\param[in] y2
y coordinate of the point associated with s2.
\param[out] k
Result of G1.exp(P2, private key f).
\param[out] l
Result of G1.exp(P2, random r).
\param[out] e
Result of G1.exp(P1, random r).
\param[out] counter
A value associated with the random r. Should be initialized with zero.

\returns ::EpidStatus

\see Tpm2CreateContext
*/
EpidStatus Tpm2Commit(Tpm2Ctx* ctx, EcPoint const* p1, void const* s2,
                      size_t s2_len, FfElement const* y2, EcPoint* k,
                      EcPoint* l, EcPoint* e, uint16_t* counter);

/*! @} */

#endif  // EPID_MEMBER_TPM2_COMMIT_H_
