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
/// TPM internal state.
/*! \file */

#ifndef EPID_MEMBER_TPM2_IBM_TSS_STATE_H_
#define EPID_MEMBER_TPM2_IBM_TSS_STATE_H_

#include "epid/common/types.h"
#include "tss2/TPM_Types.h"

/// \cond
typedef struct Epid2Params_ Epid2Params_;
typedef struct FfElement FfElement;
typedef struct TSS_CONTEXT TSS_CONTEXT;
/// \endcond

/// TPM TSS context definition
typedef struct Tpm2Ctx {
  TSS_CONTEXT* tss;                  ///< TSS context
  Epid2Params_ const* epid2_params;  ///< Intel(R) EPID 2.0 params
  TPM_HANDLE key_handle;             ///< Handle to f value of private key
  HashAlg hash_alg;                  ///< Hash algorithm to use
} Tpm2Ctx;

#endif  // EPID_MEMBER_TPM2_IBM_TSS_STATE_H_
