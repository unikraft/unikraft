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
/// Types in the native format
/*! \file */
#ifndef EPID_MEMBER_TINY_SRC_NATIVE_TYPES_H_
#define EPID_MEMBER_TINY_SRC_NATIVE_TYPES_H_

#include "epid/common/types.h"
#include "epid/member/tiny/math/mathtypes.h"

/// Group public key in unserialized format
typedef struct NativeGroupPubKey {
  GroupId gid;    ///< group ID
  EccPointFq h1;  ///< an element in G1
  EccPointFq h2;  ///< an element in G1
  EccPointFq2 w;  ///< an element in G2
} NativeGroupPubKey;

/// Membership credential in unserialized format
typedef struct NativeMembershipCredential {
  GroupId gid;   ///< group ID
  EccPointFq A;  ///< an element in G1
  FpElem x;      ///< an integer between [0, p-1]
} NativeMembershipCredential;

/// Private key in unserialized format
typedef struct NativePrivKey {
  NativeMembershipCredential cred;  ///< membership credential
  FpElem f;                         ///< an integer between [0, p-1]
} NativePrivKey;

/// Precomputed pairing values
typedef struct NativeMemberPrecomp {
  Fq12Elem e12;  ///< an element in GT
  Fq12Elem e22;  ///< an element in GT
  Fq12Elem e2w;  ///< an element in GT
  Fq12Elem ea2;  ///< an element in GT
} NativeMemberPrecomp;

/// Intel(R) EPID Basic Signature
typedef struct NativeBasicSignature {
  EccPointFq B;  ///< an element in G1
  EccPointFq K;  ///< an element in G1
  EccPointFq T;  ///< an element in G1
  FpElem c;      ///< an integer between [0, p-1]
  FpElem sx;     ///< an integer between [0, p-1]
  FpElem sf;     ///< an integer between [0, p-1]
  FpElem sa;     ///< an integer between [0, p-1]
  FpElem sb;     ///< an integer between [0, p-1]
} NativeBasicSignature;
#endif  // EPID_MEMBER_TINY_SRC_NATIVE_TYPES_H_
