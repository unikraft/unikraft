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
/// Validate native types
/*! \file */

#include "epid/member/tiny/src/validate.h"
#include "epid/member/tiny/math/efq.h"
#include "epid/member/tiny/math/efq2.h"
#include "epid/member/tiny/math/fp.h"
#include "epid/member/tiny/math/fq12.h"
#include "epid/member/tiny/math/mathtypes.h"
#include "epid/member/tiny/math/pairing.h"
#include "epid/member/tiny/src/native_types.h"
#include "epid/member/tiny/stdlib/tiny_stdlib.h"

static EccPointFq2 const epid20_g2 = {
    {{{{0xBF282394, 0xF6021343, 0x3D32470E, 0xD25D5268, 0x743CCF22, 0x21670413,
        0x4AA3DA05, 0xE20171C5}}},
     {{{0xBAA189BE, 0x7DF7B212, 0x289653E2, 0x43433BF6, 0x4FBB5656, 0x46CCDC25,
        0x53A85A80, 0x592D1EF6}}}},
    {{{{0xDD2335AE, 0x414DB822, 0x4D916838, 0x55E8B59A, 0x312826BD, 0xC621E703,
        0x51FFD350, 0xAE60A4E7}}},
     {{{0x51B92421, 0x2C90FE89, 0x9093D613, 0x2CDC6181, 0x7645E253, 0xF80274F8,
        0x89AFE5AD, 0x1AB442F9}}}}};

static EccPointFq const epid20_g1 = {
    {{0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
      0x00000000, 0x00000000}},
    {{0x00000002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
      0x00000000, 0x00000000}}};

int GroupPubKeyIsInRange(NativeGroupPubKey const* input) {
  return EFqOnCurve(&input->h1) && EFqOnCurve(&input->h2) &&
         EFq2OnCurve(&input->w);
}

int MembershipCredentialIsInRange(NativeMembershipCredential const* input) {
  return EFqOnCurve(&input->A) && FpInField(&input->x);
}

int MembershipCredentialIsInGroup(NativeMembershipCredential const* input,
                                  FpElem const* f,
                                  NativeGroupPubKey const* pubkey,
                                  PairingState const* pairing_state) {
  EccPointJacobiFq g1;
  EccPointJacobiFq t2;
  EccPointFq t2_affine;
  Fq12Elem t3;
  Fq12Elem t4;
  // to save stack space, reuse t4 and parts of t3
  EccPointFq2* t1_affine = (EccPointFq2*)&t4;
  // the following casts rely on EccPointJacobiFq2 and Fq6Elem having same size
  EccPointJacobiFq2* w = (EccPointJacobiFq2*)&t3.z0;
  EccPointJacobiFq2* t1 = (EccPointJacobiFq2*)&t3.z1;
  int result = 0;
  if (0 != memcmp(&input->gid, &pubkey->gid, sizeof(pubkey->gid))) {
    return 0;
  }
  EFqFromAffine(&g1, &epid20_g1);
  EFq2FromAffine(t1, &epid20_g2);
  EFq2FromAffine(w, &pubkey->w);
  EFq2MulSSCM(t1, t1, &input->x);
  EFq2Add(t1, t1, w);
  EFq2ToAffine(t1_affine, t1);
  PairingCompute(&t3, &input->A, t1_affine, pairing_state);
  EFqFromAffine(&t2, &pubkey->h1);
  EFqMulSSCM(&t2, &t2, f);
  EFqAdd(&t2, &t2, &g1);
  EFqToAffine(&t2_affine, &t2);
  PairingCompute(&t4, &t2_affine, &epid20_g2, pairing_state);
  result = Fq12Eq(&t3, &t4);
  memset(&t2, 0, sizeof(t2));
  memset(&t2_affine, 0, sizeof(t2_affine));
  memset(&t4, 0, sizeof(t4));
  return result;
}

int PrivKeyIsInRange(NativePrivKey const* input) {
  return EFqOnCurve(&input->cred.A) && FpInField(&input->f) &&
         FpInField(&input->cred.x);
}

int PrivKeyIsInGroup(NativePrivKey const* input,
                     NativeGroupPubKey const* pubkey,
                     PairingState const* pairing_state) {
  return MembershipCredentialIsInGroup(&input->cred, &input->f, pubkey,
                                       pairing_state);
}
