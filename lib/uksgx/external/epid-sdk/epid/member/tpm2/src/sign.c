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
/// Tpm2Sign implementation.
/*! \file  */

#include "epid/member/tpm2/sign.h"

#include <string.h>

#include "epid/common/math/finitefield.h"
#include "epid/common/src/epid2params.h"
#include "epid/common/src/hashsize.h"
#include "epid/common/src/memory.h"
#include "epid/common/types.h"
#include "epid/member/tpm2/src/state.h"

/// Handle Intel(R) EPID Error with Break
#define BREAK_ON_EPID_ERROR(ret) \
  if (kEpidNoErr != (ret)) {     \
    break;                       \
  }

static EpidStatus GetCommitNonce(Tpm2Ctx* ctx, uint16_t counter,
                                 FfElement** r) {
  if (!ctx || counter == 0 || !r) {
    return kEpidBadArgErr;
  }
  if (counter >= MAX_COMMIT_COUNT) {
    return kEpidBadArgErr;
  }
  *r = ctx->commit_data[counter - 1];
  return kEpidNoErr;
}

static void ClearCommitNonce(Tpm2Ctx* ctx, uint16_t counter) {
  if (ctx && counter > 0 && counter < MAX_COMMIT_COUNT) {
    DeleteFfElement(&ctx->commit_data[counter - 1]);
  }
}

EpidStatus Tpm2Sign(Tpm2Ctx* ctx, void const* digest, size_t digest_len,
                    uint16_t counter, FfElement* k, FfElement* s) {
  EpidStatus sts = kEpidErr;
  FfElement* t = NULL;
  BigNum* digest_bn = NULL;
  FfElement* commit_nonce = NULL;

  if (!ctx || !digest || !s || !ctx->epid2_params) {
    return kEpidBadArgErr;
  }
  if (0 == digest_len || EpidGetHashSize(ctx->hash_alg) != digest_len) {
    return kEpidBadArgErr;
  }
  if (!ctx->f) {
    return kEpidBadArgErr;
  }

  do {
    FpElemStr tmp_str;
    FiniteField* Fp = ctx->epid2_params->Fp;
    const FpElemStr zero = {0};

    sts = GetCommitNonce(ctx, counter, &commit_nonce);
    BREAK_ON_EPID_ERROR(sts);

    sts = NewBigNum(digest_len, &digest_bn);
    BREAK_ON_EPID_ERROR(sts);
    sts = NewFfElement(Fp, &t);
    BREAK_ON_EPID_ERROR(sts);

    // a. set T = digest (mod p)
    sts = ReadBigNum(digest, digest_len, digest_bn);
    BREAK_ON_EPID_ERROR(sts);
    sts = InitFfElementFromBn(Fp, digest_bn, t);
    BREAK_ON_EPID_ERROR(sts);
    // b. compute integer s = (r + T*f)(mod p)
    sts = FfMul(Fp, ctx->f, t, s);
    BREAK_ON_EPID_ERROR(sts);
    sts = FfAdd(Fp, commit_nonce, s, s);
    BREAK_ON_EPID_ERROR(sts);

    // d. if s = 0, output failure (negligible probability)
    sts = WriteFfElement(Fp, s, &tmp_str, sizeof(tmp_str));
    BREAK_ON_EPID_ERROR(sts);
    if (0 == memcmp(&zero, &tmp_str, sizeof(tmp_str))) {
      sts = kEpidBadArgErr;
      break;
    }

    if (k) {
      // k = T
      sts = WriteFfElement(Fp, t, &tmp_str, sizeof(tmp_str));
      BREAK_ON_EPID_ERROR(sts);
      sts = ReadFfElement(Fp, &tmp_str, sizeof(tmp_str), k);
      BREAK_ON_EPID_ERROR(sts);
    }
    ClearCommitNonce(ctx, counter);
    sts = kEpidNoErr;
  } while (0);

  DeleteFfElement(&t);
  DeleteBigNum(&digest_bn);

  return sts;
}

EpidStatus Tpm2ReleaseCounter(Tpm2Ctx* ctx, uint16_t counter) {
  if (!ctx) {
    return kEpidBadArgErr;
  }

  ClearCommitNonce(ctx, counter);
  return kEpidNoErr;
}
