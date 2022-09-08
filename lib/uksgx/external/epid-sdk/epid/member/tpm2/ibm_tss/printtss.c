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
/// TPM context implementation.
/*! \file */
#ifndef EPID_ENABLE_DEBUG_PRINT
#define EPID_ENABLE_DEBUG_PRINT
#endif

#include "epid/member/tpm2/ibm_tss/printtss.h"
#include <tss2/TPM_Types.h>
#include <tss2/tss.h>
#include <tss2/tssresponsecode.h>

void print_tpm2_response_code(char const* operation, TPM_RC rc) {
  const char* msg;
  const char* submsg;
  const char* num;
  TSS_ResponseCode_toString(&msg, &submsg, &num, rc);
  printf("%s: %s%s%s\n", operation, msg, submsg, num);
}

#ifdef EPID_ENABLE_DEBUG_PRINT
#undef EPID_ENABLE_DEBUG_PRINT
#endif
