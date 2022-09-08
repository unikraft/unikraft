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
/// Tiny member RegisterBaseName implementation.
/*! \file */

#define EXPORT_EPID_APIS
#include "epid/member/api.h"
#include "epid/member/tiny/src/allowed_basenames.h"
#include "epid/member/tiny/src/context.h"

EpidStatus EPID_API EpidRegisterBasename(MemberCtx* ctx, void const* basename,
                                         size_t basename_len) {
  if (basename_len == 0) {
    return kEpidBadArgErr;
  }
  if (!ctx || !basename) {
    return kEpidBadArgErr;
  }

  if (IsBasenameAllowed(ctx->allowed_basenames, basename, basename_len)) {
    return kEpidDuplicateErr;
  }
  return AllowBasename(ctx->allowed_basenames, basename, basename_len)
             ? kEpidNoErr
             : kEpidNoMemErr;
}

EpidStatus EPID_API EpidClearRegisteredBasenames(MemberCtx* ctx) {
  if (!ctx) {
    return kEpidBadArgErr;
  }
  InitBasenames(ctx->allowed_basenames, MAX_ALLOWED_BASENAMES);
  return kEpidNoErr;
}
