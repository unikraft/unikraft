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
/// SDK TPM API.
/*! \file */

#ifndef EPID_MEMBER_TPM2_CONTEXT_H_
#define EPID_MEMBER_TPM2_CONTEXT_H_

#include "epid/common/bitsupplier.h"
#include "epid/common/errors.h"
#include "epid/common/types.h"

/// \cond
typedef struct Tpm2Ctx Tpm2Ctx;
typedef struct FpElemStr FpElemStr;
typedef struct Epid2Params_ Epid2Params_;
typedef struct MemberParams MemberParams;
/// \endcond

/*!
  \addtogroup Tpm2Module tpm2
  \ingroup EpidMemberModule
  @{
*/

/// Creates a new Tpm context
/*!

 Must be called to create the TPM context that is used by other TPM
 APIs.

 You need to use a cryptographically secure random number generator
 to create a TPM context. The ::BitSupplier is provided as a function
 prototype for your own implementation of the random number generator.

 ::Tpm2DeleteContext must be called to safely release the TPM context.

 \param[in] params
 member parameters to initialize rnd_func, rnd_param, ff_elem, ctx.

 \param[in] epid2_params
 The field and group parameters.

 \param[out] rnd_func
 random function if exists in MemberParms

 \param[out] rnd_param
  random parameters if exists in MemberParms

 \param[out] f
 seed f if exists in MemberParams

 \param[out] ctx
 Newly constructed TPM context.

 \returns ::EpidStatus

 \see Tpm2DeleteContext
*/
EpidStatus Tpm2CreateContext(MemberParams const* params,
                             Epid2Params_ const* epid2_params,
                             BitSupplier* rnd_func, void** rnd_param,
                             const FpElemStr** f, Tpm2Ctx** ctx);

/// Deletes an existing Tpm context.
/*!

 Must be called to safely release a TPM context created using
 ::Tpm2CreateContext.

 De-initializes the context, frees memory used by the context, and
 sets the context pointer to NULL.

 \param[in,out] ctx
 The TPM context. Can be NULL.

 \see Tpm2CreateContext
*/
void Tpm2DeleteContext(Tpm2Ctx** ctx);

/// Sets the hash algorithm to be used by a TPM2.
/*!

 \param[in] ctx
 The TPM2 context.
 \param[in] hash_alg
 The hash algorithm to use.

 \returns ::EpidStatus
*/
EpidStatus Tpm2SetHashAlg(Tpm2Ctx* ctx, HashAlg hash_alg);

/// Reset an existing Tpm context.
/*!

Must be called to reset a TPM context created using
::Tpm2CreateContext.

Re-initializes the context, reset memory used for primary key.

\param[in,out] ctx
The TPM context. Can be NULL.

\see Tpm2CreateContext
*/
void Tpm2ResetContext(Tpm2Ctx** ctx);

/*! @} */

#endif  // EPID_MEMBER_TPM2_CONTEXT_H_
