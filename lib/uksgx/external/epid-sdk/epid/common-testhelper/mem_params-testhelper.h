/*############################################################################
  # Copyright 2016-2017 Intel Corporation
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

/*!
 * \file
 * \brief Intel(R) EPID 2.0 parameters C++ wrapper interface.
 */
#ifndef EPID_COMMON_TESTHELPER_MEM_PARAMS_TESTHELPER_H_
#define EPID_COMMON_TESTHELPER_MEM_PARAMS_TESTHELPER_H_

extern "C" {
#include "epid/common/bitsupplier.h"
#include "epid/common/types.h"
#ifdef TPM_TSS
#include "epid/member/tpm_member.h"
#else
#include "epid/member/software_member.h"
#endif
}
/// Implementation specific configuration parameters.
typedef struct MemberParams MemberParams;

/// Set MemmberParams structure
/*!

MemberParams had different structure between TPM_TSS build
and non TPM_TSS build

\returns ::void

*/
void SetMemberParams(BitSupplier rnd_func, void* rnd_param, const FpElemStr* f,
                     MemberParams* params);

#endif  // EPID_COMMON_TESTHELPER_MEM_PARAMS_TESTHELPER_H_
