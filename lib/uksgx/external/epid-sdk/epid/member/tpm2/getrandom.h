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
#ifndef EPID_MEMBER_TPM2_GETRANDOM_H_
#define EPID_MEMBER_TPM2_GETRANDOM_H_

/*!
 * \file
 * \brief SDK TPM API.
 */

#include "epid/common/errors.h"

/// \cond
typedef struct Tpm2Ctx Tpm2Ctx;
/// \endcond

/*!
\addtogroup Tpm2Module tpm2
\ingroup EpidMemberModule
@{
*/

/// Get random data
/*!
This command returns the next num_bits from the random number generator (RNG).

\param[in,out] ctx
TPM context.

\param[in] num_bits
Number of bits to return.

\param[out] random_data
Output random bits.

\returns ::EpidStatus

\see Tpm2CreateContext
*/
EpidStatus Tpm2GetRandom(Tpm2Ctx* ctx, int const num_bits, void* random_data);

/*! @} */

#endif  // EPID_MEMBER_TPM2_GETRANDOM_H_
