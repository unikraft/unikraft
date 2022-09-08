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
/// TPM internal state
/*! \file */

#ifndef EPID_MEMBER_TPM2_SRC_STATE_H_
#define EPID_MEMBER_TPM2_SRC_STATE_H_

#include <stddef.h>
#include "epid/common/bitsupplier.h"
#include "epid/common/stdtypes.h"
#include "epid/common/types.h"  // HashAlg

/// \cond
typedef struct Epid2Params_ Epid2Params_;
typedef struct FfElement FfElement;
/// \endcond

/// Maximum NV index
#define MAX_NV_NUMBER 10
/// Minimal possible NV index in TPM
#define MIN_NV_INDEX 0x01000000
/// Maximum number of Tpm2Commit random values that can exist in memory
/// simultaneously
#define MAX_COMMIT_COUNT 100

#if (MAX_COMMIT_COUNT >= UINT16_MAX)
#error "MAX_COMMIT_COUNT maximum commit count is restricted by uint16_t"
#endif

/// One NV entry
typedef struct NvEntry {
  uint32_t nv_index;
  void* data;
  size_t data_size;
} NvEntry;

/// TPM State
typedef struct Tpm2Ctx {
  Epid2Params_ const* epid2_params;  ///< Intel(R) EPID 2.0 params
  FfElement* f;                      ///< Member private key f value
  BitSupplier rnd_func;  ///< Pseudo random number generation function
  void* rnd_param;       ///< Pointer to user context for rnd_func
  HashAlg hash_alg;      ///< Hash algorithm to use
  FfElement* commit_data[MAX_COMMIT_COUNT];  ///< Tpm2Commit random value
                                             ///< corresponding to counter
  NvEntry nv[MAX_NV_NUMBER];                 ///< NV memory
} Tpm2Ctx;

#endif  // EPID_MEMBER_TPM2_SRC_STATE_H_
