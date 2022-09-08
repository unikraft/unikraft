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
/// Tiny portable implementations of standard library functions
/*! \file */

#include "epid/member/tiny/src/serialize.h"
#include <stddef.h>

#include "epid/common/types.h"
#include "epid/member/tiny/math/serialize.h"
#include "epid/member/tiny/src/native_types.h"
#include "epid/member/tiny/src/signbasic.h"

void* BasicSignatureSerialize(BasicSignature* dest,
                              NativeBasicSignature const* src) {
#if defined(UNOPTIMIZED_SERIALIZATION)
  EFqSerialize(&dest->B, &src->B);
  EFqSerialize(&dest->K, &src->K);
  EFqSerialize(&dest->T, &src->T);
  FpSerialize(&dest->c, &src->c);
  FpSerialize(&dest->sx, &src->sx);
  FpSerialize(&dest->sf, &src->sf);
  FpSerialize(&dest->sa, &src->sa);
  FpSerialize(&dest->sb, &src->sb);
  return dest + 1;
#else   // defined(UNOPTIMIZED_SERIALIZATION)
  SwapNativeAndPortableLayout(dest, sizeof(*dest), src, sizeof(*src));
  return dest + 1;
#endif  // defined(UNOPTIMIZED_SERIALIZATION)
}

void const* BasicSignatureDeserialize(NativeBasicSignature* dest,
                                      BasicSignature const* src) {
#if defined(UNOPTIMIZED_SERIALIZATION)
  EFqDeserialize(&dest->B, &src->B);
  EFqDeserialize(&dest->K, &src->K);
  EFqDeserialize(&dest->T, &src->T);
  FpDeserialize(&dest->c, &src->c);
  FpDeserialize(&dest->sx, &src->sx);
  FpDeserialize(&dest->sf, &src->sf);
  FpDeserialize(&dest->sa, &src->sa);
  FpDeserialize(&dest->sb, &src->sb);
  return src + 1;
#else   // defined(UNOPTIMIZED_SERIALIZATION)
  SwapNativeAndPortableLayout(dest, sizeof(*dest), src, sizeof(*src));
  return src + 1;
#endif  // defined(UNOPTIMIZED_SERIALIZATION)
}

void* GroupPubKeySerialize(GroupPubKey* dest, NativeGroupPubKey const* src) {
#if defined(UNOPTIMIZED_SERIALIZATION)
  EFqSerialize(&dest->h1, &src->h1);
  EFqSerialize(&dest->h2, &src->h2);
  EFq2Serialize(&dest->w, &src->w);
  dest->gid = src->gid;
  return dest + 1;
#else   // defined(UNOPTIMIZED_SERIALIZATION)
  dest->gid = src->gid;
  SwapNativeAndPortableLayout(
      (uint8_t*)dest + sizeof(dest->gid), sizeof(*dest) - sizeof(dest->gid),
      (uint8_t*)src + sizeof(src->gid), sizeof(*src) - sizeof(src->gid));
  return dest + 1;
#endif  // defined(UNOPTIMIZED_SERIALIZATION)
}

void const* GroupPubKeyDeserialize(NativeGroupPubKey* dest,
                                   GroupPubKey const* src) {
#if defined(UNOPTIMIZED_SERIALIZATION)
  EFqDeserialize(&dest->h1, &src->h1);
  EFqDeserialize(&dest->h2, &src->h2);
  EFq2Deserialize(&dest->w, &src->w);
  dest->gid = src->gid;
  return src + 1;
#else   // defined(UNOPTIMIZED_SERIALIZATION)
  dest->gid = src->gid;
  SwapNativeAndPortableLayout(
      (uint8_t*)dest + sizeof(dest->gid), sizeof(*dest) - sizeof(dest->gid),
      (uint8_t*)src + sizeof(src->gid), sizeof(*src) - sizeof(src->gid));
  return src + 1;
#endif  // defined(UNOPTIMIZED_SERIALIZATION)
}

void const* PrivKeyDeserialize(NativePrivKey* dest, PrivKey const* src) {
#if defined(UNOPTIMIZED_SERIALIZATION)
  EFqDeserialize(&dest->cred.A, &src->A);
  FpDeserialize(&dest->cred.x, &src->x);
  FpDeserialize(&dest->f, &src->f);
  dest->cred.gid = src->gid;
  return src + 1;
#else   // defined(UNOPTIMIZED_SERIALIZATION)
  dest->cred.gid = src->gid;
  SwapNativeAndPortableLayout((uint8_t*)dest + sizeof(dest->cred.gid),
                              sizeof(*dest) - sizeof(dest->cred.gid),
                              (uint8_t*)src + sizeof(src->gid),
                              sizeof(*src) - sizeof(src->gid));
  return src + 1;
#endif  // defined(UNOPTIMIZED_SERIALIZATION)
}

void const* PreCompDeserialize(NativeMemberPrecomp* dest,
                               MemberPrecomp const* src) {
#if defined(UNOPTIMIZED_SERIALIZATION)
  Fq12Deserialize(&dest->ea2, (Fq12ElemStr*)&src->ea2);
  Fq12Deserialize(&dest->e12, (Fq12ElemStr*)&src->e12);
  Fq12Deserialize(&dest->e22, (Fq12ElemStr*)&src->e22);
  Fq12Deserialize(&dest->e2w, (Fq12ElemStr*)&src->e2w);
  return src + 1;
#else   // defined(UNOPTIMIZED_SERIALIZATION)
  SwapNativeAndPortableLayout(dest, sizeof(*dest), src, sizeof(*src));
  return src + 1;
#endif  // defined(UNOPTIMIZED_SERIALIZATION)
}

void const* MembershipCredentialDeserialize(NativeMembershipCredential* dest,
                                            MembershipCredential const* src) {
#if defined(UNOPTIMIZED_SERIALIZATION)
  EFqDeserialize(&dest->A, &src->A);
  FpDeserialize(&dest->x, &src->x);
  dest->gid = src->gid;
  return src + 1;
#else   // defined(UNOPTIMIZED_SERIALIZATION)
  dest->gid = src->gid;
  SwapNativeAndPortableLayout(
      (uint8_t*)dest + sizeof(dest->gid), sizeof(*dest) - sizeof(dest->gid),
      (uint8_t*)src + sizeof(src->gid), sizeof(*src) - sizeof(src->gid));
  return src + 1;
#endif  // defined(UNOPTIMIZED_SERIALIZATION)
}
