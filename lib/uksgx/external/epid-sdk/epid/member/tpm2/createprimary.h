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
/// TPM2_CreatePrimary command interface.
/*! \file */
#ifndef EPID_MEMBER_TPM2_CREATEPRIMARY_H_
#define EPID_MEMBER_TPM2_CREATEPRIMARY_H_

#include "epid/common/errors.h"

/// \cond
typedef struct Tpm2Ctx Tpm2Ctx;
typedef struct G1ElemStr G1ElemStr;
/// \endcond

/// Creates Primary key
/*!

\param[in,out] ctx
TPM context.
\param[out] p_str
Primary key: g1^f
\returns ::EpidStatus
*/
EpidStatus Tpm2CreatePrimary(Tpm2Ctx* ctx, G1ElemStr* p_str);

#endif  // EPID_MEMBER_TPM2_CREATEPRIMARY_H_
