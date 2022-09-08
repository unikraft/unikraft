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
/// Tiny member SetHashAlg implementation.
/*! \file */

#define EXPORT_EPID_APIS
#include <epid/member/api.h>

#include "epid/member/tiny/src/context.h"

EpidStatus EPID_API EpidMemberSetHashAlg(MemberCtx* ctx, HashAlg hash_alg) {
  if (!ctx) {
    return kEpidBadArgErr;
  }
  if ((kSha512 != hash_alg) && (kSha256 != hash_alg)) {
    return kEpidBadArgErr;
  }
  ctx->hash_alg = hash_alg;
  return kEpidNoErr;
}
