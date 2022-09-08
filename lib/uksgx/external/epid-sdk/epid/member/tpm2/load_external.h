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

#ifndef EPID_MEMBER_TPM2_LOAD_EXTERNAL_H_
#define EPID_MEMBER_TPM2_LOAD_EXTERNAL_H_

#include "epid/common/errors.h"
#include "epid/common/types.h"  // HashAlg

/// \cond
typedef struct Tpm2Ctx Tpm2Ctx;
typedef struct FpElemStr FpElemStr;
/// \endcond

/*!
 \addtogroup Tpm2Module tpm2
 \ingroup EpidMemberModule
 @{
*/

/// Invokes TPM2_LoadExternal command
/*!
 This command is used to load an object that is not a Protected Object into the
 TPM. The command allows loading of a public area or both a public and sensitive
 area.

 \param[in,out] ctx
 TPM context.

 \param[in] f_str
 The f value of the member private key.

 \returns ::EpidStatus
*/
EpidStatus Tpm2LoadExternal(Tpm2Ctx* ctx, FpElemStr const* f_str);

/*! @} */

#endif  // EPID_MEMBER_TPM2_LOAD_EXTERNAL_H_
