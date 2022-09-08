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
/// Tpm2Commit implementation.
/*! \file */

#include "epid/member/tpm2/commit.h"
#include "epid/common/math/ecgroup.h"
#include "epid/common/math/finitefield.h"
#include "epid/common/src/epid2params.h"
#include "epid/common/src/memory.h"
#include "epid/member/tpm2/src/state.h"

/// Handle Intel(R) EPID Error with Break
#define BREAK_ON_EPID_ERROR(ret) \
  if (kEpidNoErr != (ret)) {     \
    break;                       \
  }

static EpidStatus InsertR(Tpm2Ctx* ctx, FfElement* r, uint16_t* counter) {
  uint16_t index = 0;
  if (!ctx || !r || !counter) {
    return kEpidBadArgErr;
  }
  for (index = 0; index < MAX_COMMIT_COUNT; ++index) {
    if (!ctx->commit_data[index]) {
      ctx->commit_data[index] = r;
      break;
    }
  }
  if (index == MAX_COMMIT_COUNT) {
    return kEpidMemAllocErr;
  }
  *counter = index + 1;  // counter == 0 should be invalid
  return kEpidNoErr;
}

static void DeleteR(Tpm2Ctx* ctx, uint16_t* counter) {
  if (counter && *counter != 0 && *counter <= MAX_COMMIT_COUNT) {
    DeleteFfElement(&ctx->commit_data[*counter - 1]);
    *counter = 0;
  }
}

EpidStatus Tpm2Commit(Tpm2Ctx* ctx, EcPoint const* p1, void const* s2,
                      size_t s2_len, FfElement const* y2, EcPoint* k,
                      EcPoint* l, EcPoint* e, uint16_t* counter) {
  EpidStatus sts = kEpidErr;
  FiniteField* Fp = NULL;
  FiniteField* Fq = NULL;
  EcGroup* G1 = NULL;
  FfElement* x2 = NULL;
  FfElement* r = NULL;
  EcPoint* point = NULL;
  EcPoint* infinity = NULL;
  uint16_t ctr = 0;

  if (!ctx || !ctx->epid2_params || !ctx->f) {
    return kEpidBadArgErr;
  }

  if (s2 && s2_len <= 0) {
    return kEpidBadArgErr;
  }

  if ((!s2 && y2) || (s2 && !y2)) {
    return kEpidBadArgErr;
  }

  if (s2 && (!k || !l)) {
    return kEpidBadArgErr;
  }

  if (!e || !counter) {
    return kEpidBadArgErr;
  }

  do {
    G1ElemStr point_str = {0};
    BigNumStr r_str = {0};
    const BigNumStr kOne = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
    bool is_equal = false;
    Fp = ctx->epid2_params->Fp;
    Fq = ctx->epid2_params->Fq;
    G1 = ctx->epid2_params->G1;

    sts = NewEcPoint(G1, &infinity);
    BREAK_ON_EPID_ERROR(sts);
    // step b
    if (s2) {
      // step c: compute x2 := HnameAlg(s2) mod p
      sts = NewFfElement(Fq, &x2);
      BREAK_ON_EPID_ERROR(sts);
      sts = FfHash(Fq, s2, s2_len, ctx->hash_alg, x2);
      BREAK_ON_EPID_ERROR(sts);

      // step d: if (x2, y2) is not a point on the curve of signHandle, return
      // TPM_RC_ECC_POINT
      sts = NewEcPoint(G1, &point);
      BREAK_ON_EPID_ERROR(sts);
      sts = WriteFfElement(Fq, x2, &point_str.x, sizeof(point_str.x));
      BREAK_ON_EPID_ERROR(sts);
      sts = WriteFfElement(Fq, y2, &point_str.y, sizeof(point_str.y));
      BREAK_ON_EPID_ERROR(sts);
      sts = ReadEcPoint(G1, &point_str, sizeof(point_str), point);
      BREAK_ON_EPID_ERROR(sts);
    }

    // step e: if p1 is not an Empty Point and p1 is not a point on the curve of
    // signHandle, return TPM_RC_ECC_POINT
    // This step is guranteed by ReadEcPoint SDK function

    // step g: generate or derive r (see C.2.2)
    // step h: set r = r mod n
    sts = NewFfElement(Fp, &r);
    BREAK_ON_EPID_ERROR(sts);
    sts = FfGetRandom(Fp, &kOne, ctx->rnd_func, ctx->rnd_param, r);
    BREAK_ON_EPID_ERROR(sts);
    sts = InsertR(ctx, r, &ctr);
    BREAK_ON_EPID_ERROR(sts);
    sts = WriteFfElement(Fp, r, &r_str, sizeof(r_str));
    BREAK_ON_EPID_ERROR(sts);
    // step i: if s2 is not an Empty Buffer, set K = [ds](x2, y2) and L =
    // [r](x2, y2)
    if (s2) {
      BigNumStr f_str = {0};
      do {
        sts = WriteFfElement(Fp, ctx->f, &f_str, sizeof(f_str));
        BREAK_ON_EPID_ERROR(sts);
        sts = EcExp(G1, point, &f_str, k);
        BREAK_ON_EPID_ERROR(sts);
      } while (0);
      EpidZeroMemory(&f_str, sizeof(f_str));
      BREAK_ON_EPID_ERROR(sts);
      sts = EcExp(G1, point, &r_str, l);
      BREAK_ON_EPID_ERROR(sts);
      sts = EcIsEqual(G1, k, infinity, &is_equal);
      BREAK_ON_EPID_ERROR(sts);
      if (is_equal) {
        sts = kEpidBadArgErr;
        break;
      }
      sts = EcIsEqual(G1, l, infinity, &is_equal);
      BREAK_ON_EPID_ERROR(sts);
      if (is_equal) {
        sts = kEpidBadArgErr;
        break;
      }
    }
    // step j: if p1 is not an Empty Point, set E = [r](p1 )
    if (p1) {
      sts = EcExp(G1, p1, &r_str, e);
      BREAK_ON_EPID_ERROR(sts);
    } else {
      // step k: if p1 is an Empty Point and s2 is an Empty Buffer, set E = [r]G
      sts = EcExp(G1, ctx->epid2_params->g1, &r_str, e);
      BREAK_ON_EPID_ERROR(sts);
    }
    sts = EcIsEqual(G1, e, infinity, &is_equal);
    BREAK_ON_EPID_ERROR(sts);
    if (is_equal) {
      sts = kEpidBadArgErr;
      break;
    }
    *counter = ctr;
  } while (0);

  if (sts != kEpidNoErr) {
    DeleteR(ctx, &ctr);
  }
  DeleteEcPoint(&infinity);
  DeleteEcPoint(&point);
  DeleteFfElement(&x2);
  return sts;
}
