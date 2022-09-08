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

/*!
 * \file
 * \brief Implementation of Intel(R) EPID specific predicates for Set MemParams
 * strcuture
 */
#include "epid/common-testhelper/mem_params-testhelper.h"

void SetMemberParams(BitSupplier rnd_func, void* rnd_param, const FpElemStr* f,
                     MemberParams* params) {
#ifdef TPM_TSS
  (void)rnd_func;
  (void)rnd_param;
  params->f = f;
#else
  params->rnd_func = rnd_func;
  params->rnd_param = rnd_param;
  params->f = f;
#endif
}
