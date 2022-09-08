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
/// Precomputed signature management implementation.
/*! \file */

#define EXPORT_EPID_APIS
#include <epid/member/api.h>

EpidStatus EPID_API EpidAddPreSigs(MemberCtx* ctx, size_t number_presigs) {
  (void)ctx;
  (void)number_presigs;
  return kEpidNotImpl;
}

size_t EPID_API EpidGetNumPreSigs(MemberCtx const* ctx) {
  (void)ctx;
  return 0;
}
