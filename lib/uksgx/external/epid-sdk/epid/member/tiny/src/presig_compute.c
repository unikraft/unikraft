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
/// Precomputed signature computation.
/*! \file */

#include "epid/member/tiny/src/presig_compute.h"

#include "epid/member/tiny/math/efq.h"
#include "epid/member/tiny/math/fp.h"
#include "epid/member/tiny/math/fq12.h"
#include "epid/member/tiny/math/serialize.h"
#include "epid/member/tiny/math/vli.h"
#include "epid/member/tiny/src/context.h"

static const EccPointFq epid20_g1 = {
    {{0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
      0x00000000, 0x00000000}},
    {{0x00000002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
      0x00000000, 0x00000000}}};
static const FpElem epid20_p = {{0xD10B500D, 0xF62D536C, 0x1299921A, 0x0CDC65FB,
                                 0xEE71A49E, 0x46E5F25E, 0xFFFCF0CD,
                                 0xFFFFFFFF}};

EpidStatus EpidMemberComputePreSig(MemberCtx const* ctx,
                                   PreComputedSignatureData* presig) {
  /* B and K are not computed by this precomputation.
   *This differs from the Intel(R) EPID 2.0 spec.
   *On IoT and especially accelerated platforms,
   *the extra latency is likely less expensive
   *than the space and possibly redundant computation
   *needed to compute and store these values.
   */
  EpidStatus sts = kEpidMathErr;
  EccPointFq t;

  EccPointJacobiFq tmp1;
  EccPointJacobiFq tmp2;
  do {
    if (!FpRandNonzero(&presig->a, ctx->rnd_func, ctx->rnd_param)) {
      break;
    }

    // T = A * h2^a
    EFqDeserialize(&t, &ctx->pub_key.h2);
    EFqFromAffine(&tmp1, &t);
    EFqMulSSCM(&tmp2, &tmp1, &presig->a);
    EFqDeserialize(&t, &ctx->credential.A);
    EFqFromAffine(&tmp1, &t);
    EFqAdd(&tmp2, &tmp2, &tmp1);
    if (EFqToAffine(&presig->T, &tmp2) != 1) {
      break;
    }

    FpDeserialize((FpElem*)&t.x, &ctx->credential.x);
    FpMul(&presig->b, &presig->a, (FpElem*)&t.x);

    if (!FpRandNonzero(&presig->rx, ctx->rnd_func, ctx->rnd_param)) {
      break;
    }
    if (!FpRandNonzero(&presig->rf, ctx->rnd_func, ctx->rnd_param)) {
      break;
    }
    if (!FpRandNonzero(&presig->ra, ctx->rnd_func, ctx->rnd_param)) {
      break;
    }
    if (!FpRandNonzero(&presig->rb, ctx->rnd_func, ctx->rnd_param)) {
      break;
    }
    VliSub(&t.x.limbs, &epid20_p.limbs,
           &presig->rx.limbs);  // FpNeg(&t.x, rx), but this is fast.
    FpMul((FpElem*)&t.y, &presig->a, &presig->rx);
    FpSub((FpElem*)&t.y, &presig->rb, (FpElem*)&t.y);

    // R2 = ea2^&t.x * e12^rf * e22 ^ &t.y * e2w ^ ra
    Fq12MultiExp(&presig->R2, &ctx->precomp.ea2, &t.x.limbs, &ctx->precomp.e12,
                 &presig->rf.limbs, &ctx->precomp.e22, &t.y.limbs,
                 &ctx->precomp.e2w, &presig->ra.limbs);
    sts = kEpidNoErr;
  } while (0);

  // Zero sensitive stack variables
  FpClear((FpElem*)&t.x);
  FpClear((FpElem*)&t.y);
  EFqFromAffine(&tmp1, &epid20_g1);
  EFqFromAffine(&tmp2, &epid20_g1);
  return sts;
}
