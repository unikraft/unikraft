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
/// Tiny member SetSigRl implementation.
/*! \file */

#define EXPORT_EPID_APIS
#include <epid/member/api.h>

#include "epid/member/tiny/src/context.h"
#include "epid/member/tiny/stdlib/endian.h"
#include "epid/member/tiny/stdlib/tiny_stdlib.h"

EpidStatus EPID_API EpidMemberSetSigRl(MemberCtx* ctx, SigRl const* sig_rl,
                                       size_t sig_rl_size) {
  uint32_t n2_in = 0;
  size_t calculated_sig_rl_size = 0;
  uint32_t i = 0;
  if (!ctx || !sig_rl) {
    return kEpidBadArgErr;
  }

  if (!ctx->is_provisioned) {
    return kEpidOutOfSequenceError;
  }

  n2_in = be32toh(sig_rl->n2);

  // sanity check SigRl size
  if (n2_in > MAX_SIGRL_ENTRIES) {
    return kEpidBadArgErr;
  }
  calculated_sig_rl_size = MIN_SIGRL_SIZE + n2_in * sizeof(sig_rl->bk[0]);
  if (calculated_sig_rl_size != sig_rl_size) {
    return kEpidBadArgErr;
  }
  // verify that gid given and gid in SigRl match
  if (0 != memcmp(&ctx->pub_key.gid, &sig_rl->gid, sizeof(sig_rl->gid))) {
    return kEpidBadArgErr;
  }

  // ensure version is not being reverted
  if (ctx->sig_rl) {
    uint32_t current_ver = be32toh(ctx->sig_rl->version);
    uint32_t incoming_ver = be32toh(sig_rl->version);
    if (current_ver >= incoming_ver) {
      return kEpidBadArgErr;
    }
  }

#ifdef USE_SIGRL_BY_REFERENCE
  ctx->sig_rl = (SigRl*)sig_rl;
  (void)i;
#else
  if (!ctx->sig_rl) {
    ctx->sig_rl = (SigRl*)ctx->heap;
  }
  ctx->sig_rl->version = sig_rl->version;
  ctx->sig_rl->n2 = sig_rl->n2;
  memset(ctx->sig_rl->bk, 0, MAX_SIGRL_ENTRIES * sizeof(*ctx->sig_rl->bk));

  for (i = 0; i < n2_in; i++) {
    ctx->sig_rl->bk[i] = sig_rl->bk[i];
  }
#endif
  return kEpidNoErr;
}
