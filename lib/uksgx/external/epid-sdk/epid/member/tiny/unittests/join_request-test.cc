/*############################################################################
  # Copyright 2016-2017 Intel Corporation
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

/*!
 * \file
 * \brief Join Request related unit tests.
 */

#include <cstring>
#include <memory>
#include "epid/common-testhelper/epid_gtest-testhelper.h"
#include "gtest/gtest.h"

extern "C" {
#include "epid/common/math/ecgroup.h"
#include "epid/common/math/finitefield.h"
#include "epid/common/src/epid2params.h"
#include "epid/member/api.h"
}

#include "epid/common-testhelper/ecgroup_wrapper-testhelper.h"
#include "epid/common-testhelper/ecpoint_wrapper-testhelper.h"
#include "epid/common-testhelper/epid_params-testhelper.h"
#include "epid/common-testhelper/errors-testhelper.h"
#include "epid/common-testhelper/ffelement_wrapper-testhelper.h"
#include "epid/common-testhelper/finite_field_wrapper-testhelper.h"
#include "epid/common-testhelper/mem_params-testhelper.h"
#include "epid/common-testhelper/prng-testhelper.h"
#include "epid/member/tiny/unittests/member-testhelper.h"

/// compares FpElemStr values
bool operator==(FpElemStr const& lhs, FpElemStr const& rhs) {
  return 0 == std::memcmp(&lhs, &rhs, sizeof(lhs));
}

/// compares JoinRequest values
bool operator==(JoinRequest const& lhs, JoinRequest const& rhs) {
  return 0 == std::memcmp(&lhs, &rhs, sizeof(lhs));
}

/// compares JoinRequest values for inequality
bool operator!=(JoinRequest const& lhs, JoinRequest const& rhs) {
  return 0 != std::memcmp(&lhs, &rhs, sizeof(lhs));
}

namespace {

// local constant for Join Request tests. This can be hoisted later if needed
// avoids cpplint warning about multiple includes.
const GroupPubKey kPubKey = {
#include "epid/common-testhelper/testdata/grp01/gpubkey.inc"
};

const FpElemStr kFEps1 = {0x56, 0x57, 0xda, 0x39, 0x9f, 0x69, 0x17, 0x84,
                          0xac, 0xf9, 0xf6, 0xdf, 0xfe, 0xd2, 0x41, 0xe8,
                          0x02, 0x30, 0xf8, 0xd8, 0x72, 0x35, 0xd3, 0x0e,
                          0x76, 0x2e, 0xda, 0x4b, 0xf4, 0xc5, 0x31, 0x0f};
/// Validates join request.
void ValidateJoinRequest(JoinRequest const& request, HashAlg hash_alg,
                         GroupPubKey const& grp_public_key, FpElemStr const& f,
                         IssuerNonce const& ni) {
  Epid2Params params_values = {
#include "epid/common/src/epid2params_ate.inc"
  };

  Epid20Params params;

  // h1^f ?= F
  EcPointObj F_expected(&params.G1, grp_public_key.h1);
  THROW_ON_EPIDERR(EcExp(params.G1, F_expected, (BigNumStr*)&f, F_expected));
  ASSERT_EQ(*(G1ElemStr*)(F_expected.data().data()), request.F);

  // H(p|g1|g2|h1|h2|w|F|R|ni) ?= c, where R = h1^s * F^(-c)
  FfElementObj nc(&params.fp, request.c);
  THROW_ON_EPIDERR(FfNeg(params.fp, nc, nc));
  EcPointObj a(&params.G1, grp_public_key.h1);
  EcPointObj b(&params.G1, request.F);
  THROW_ON_EPIDERR(EcExp(params.G1, a, (BigNumStr*)&request.s, a));
  THROW_ON_EPIDERR(EcExp(params.G1, b, (BigNumStr*)nc.data().data(), b));
  THROW_ON_EPIDERR(EcMul(params.G1, a, b, a));

#pragma pack(1)
  struct {
    BigNumStr p;     // Intel(R) EPID 2.0 parameter p
    G1ElemStr g1;    // Intel(R) EPID 2.0 parameter g1
    G2ElemStr g2;    // Intel(R) EPID 2.0 parameter g2
    G1ElemStr h1;    // Group public key value h1
    G1ElemStr h2;    // Group public key value h2
    G2ElemStr w;     // Group public key value w
    G1ElemStr F;     // Variable F computed in algorithm
    G1ElemStr R;     // Variable R computed in algorithm
    IssuerNonce NI;  // Issuer Nonce
  } commitment_values = {params_values.p,
                         params_values.g1,
                         params_values.g2,
                         grp_public_key.h1,
                         grp_public_key.h2,
                         grp_public_key.w,
                         request.F,
                         *(G1ElemStr*)(a.data().data()),
                         ni};
#pragma pack()

  FfElementObj commitment(&params.fp);
  THROW_ON_EPIDERR(FfHash(params.fp, &commitment_values,
                          sizeof commitment_values, hash_alg, commitment));
  ASSERT_EQ(*(FpElemStr*)(commitment.data().data()), request.c);
}

TEST_F(EpidMemberTest, CreateJoinRequestFailsGivenNullParameters) {
  GroupPubKey pub_key = kPubKey;
  IssuerNonce ni;
  MemberParams params;
  Prng prng;
  JoinRequest join_request;
  SetMemberParams(Prng::Generate, &prng, nullptr, &params);
  MemberCtxObj ctx(&params);
  THROW_ON_EPIDERR(EpidMemberSetHashAlg(ctx, kSha512));
  EXPECT_EQ(kEpidBadArgErr,
            EpidCreateJoinRequest(nullptr, &pub_key, &ni, &join_request));
  EXPECT_EQ(kEpidBadArgErr,
            EpidCreateJoinRequest(ctx, nullptr, &ni, &join_request));
  EXPECT_EQ(kEpidBadArgErr,
            EpidCreateJoinRequest(ctx, &pub_key, nullptr, &join_request));
  EXPECT_EQ(kEpidBadArgErr, EpidCreateJoinRequest(ctx, &pub_key, &ni, nullptr));
}

TEST_F(EpidMemberTest, CreateJoinRequestFailsGivenNoF) {
  GroupPubKey pub_key = kPubKey;
  IssuerNonce ni;
  MemberParams params;
  Prng prng;
  JoinRequest join_request;
  SetMemberParams(Prng::Generate, &prng, nullptr, &params);
  MemberCtxObj ctx(&params);
  THROW_ON_EPIDERR(EpidMemberSetHashAlg(ctx, kSha512));
  EXPECT_EQ(kEpidBadArgErr,
            EpidCreateJoinRequest(ctx, &pub_key, &ni, &join_request));
}

TEST_F(EpidMemberTest, CreateJoinRequestFailsGivenInvalidGroupKey) {
  Prng prng;
  MemberParams params = {0};
  GroupPubKey pub_key = kPubKey;
  FpElemStr f = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
  };
  IssuerNonce ni = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03,
      0x04, 0x05, 0x06, 0x07, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01,
  };
  pub_key.h1.x.data.data[15] = 0xff;
  Epid20Params epid_params;
  EcPointObj pt(&epid_params.G1);
  JoinRequest join_request;
  SetMemberParams(Prng::Generate, &prng, &f, &params);
  MemberCtxObj member(&params);
  THROW_ON_EPIDERR(EpidMemberSetHashAlg(member, kSha512));
  ASSERT_NE(kEpidNoErr, ReadEcPoint(epid_params.G1, (uint8_t*)&pub_key.h1,
                                    sizeof(pub_key.h1), pt));
  EXPECT_EQ(kEpidBadArgErr,
            EpidCreateJoinRequest(member, &pub_key, &ni, &join_request));
}

TEST_F(EpidMemberTest, CreateJoinRequestFailsGivenInvalidFValue) {
  Prng prng;
  MemberParams params = {0};
  GroupPubKey pub_key = kPubKey;
  FpElemStr f = {
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
  };
  IssuerNonce ni = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03,
      0x04, 0x05, 0x06, 0x07, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01,
  };
  JoinRequest join_request;
  EpidStatus sts;
  SetMemberParams(Prng::Generate, &prng, &f, &params);

  std::unique_ptr<uint8_t[]> member;
  size_t context_size = 0;
  sts = EpidMemberGetSize(&params, &context_size);
  EXPECT_TRUE(kEpidNoErr == sts || kEpidBadArgErr == sts)
      << "Actual value " << sts;

  if (kEpidNoErr == sts) {
    member.reset(new uint8_t[context_size]());
    sts = EpidMemberInit(&params, (MemberCtx*)member.get());
    EXPECT_TRUE(kEpidNoErr == sts || kEpidBadArgErr == sts)
        << "Actual value " << sts;
  }

  if (kEpidNoErr == sts) {
    sts = EpidCreateJoinRequest((MemberCtx*)member.get(), &pub_key, &ni,
                                &join_request);
    EXPECT_EQ(kEpidBadArgErr, sts);
  }

  EpidMemberDeinit((MemberCtx*)member.get());
}

TEST_F(EpidMemberTest, CreateJoinRequestWorksUsingSha512) {
  Prng prng;
  MemberParams params = {0};
  GroupPubKey pub_key = kPubKey;
  FpElemStr f = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
  };
  IssuerNonce ni = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03,
      0x04, 0x05, 0x06, 0x07, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01,
  };
  JoinRequest join_request;
  SetMemberParams(Prng::Generate, &prng, &f, &params);
  MemberCtxObj member(&params);
  THROW_ON_EPIDERR(EpidMemberSetHashAlg(member, kSha512));
  EXPECT_EQ(kEpidNoErr,
            EpidCreateJoinRequest(member, &pub_key, &ni, &join_request));
  EXPECT_NO_FATAL_FAILURE(
      ValidateJoinRequest(join_request, kSha512, pub_key, f, ni));
}

TEST_F(EpidMemberTest, CreateJoinRequestWorksUsingSha256) {
  Prng prng;
  MemberParams params = {0};
  GroupPubKey pub_key = kPubKey;
  FpElemStr f = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
  };
  IssuerNonce ni = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03,
      0x04, 0x05, 0x06, 0x07, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01,
  };
  JoinRequest join_request;
  SetMemberParams(Prng::Generate, &prng, &f, &params);
  MemberCtxObj member(&params);
  THROW_ON_EPIDERR(EpidMemberSetHashAlg(member, kSha256));
  EXPECT_EQ(kEpidNoErr,
            EpidCreateJoinRequest(member, &pub_key, &ni, &join_request));
  EXPECT_NO_FATAL_FAILURE(
      ValidateJoinRequest(join_request, kSha256, pub_key, f, ni));
}

TEST_F(EpidMemberTest,
       CreateJoinRequestGeneratesDiffJoinRequestsOnMultipleCalls) {
  Prng prng;
  MemberParams params = {0};
  GroupPubKey pub_key = kPubKey;
  FpElemStr f = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
  };
  IssuerNonce ni = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03,
      0x04, 0x05, 0x06, 0x07, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01,
  };
  JoinRequest join_request1;
  JoinRequest join_request2;
  SetMemberParams(Prng::Generate, &prng, &f, &params);
  MemberCtxObj member(&params);
  THROW_ON_EPIDERR(EpidMemberSetHashAlg(member, kSha512));
  EXPECT_EQ(kEpidNoErr,
            EpidCreateJoinRequest(member, &pub_key, &ni, &join_request1));
  EXPECT_NO_FATAL_FAILURE(
      ValidateJoinRequest(join_request1, kSha512, pub_key, f, ni));
  EXPECT_EQ(kEpidNoErr,
            EpidCreateJoinRequest(member, &pub_key, &ni, &join_request2));
  EXPECT_NO_FATAL_FAILURE(
      ValidateJoinRequest(join_request2, kSha512, pub_key, f, ni));
  EXPECT_NE(join_request1, join_request2);
}

TEST_F(EpidMemberTest,
       CreateJoinRequestGeneratesDiffJoinRequestsGivenDiffHashAlgs) {
  MemberParams params = {0};
  GroupPubKey pub_key = kPubKey;
  FpElemStr f = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
  };
  IssuerNonce ni = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03,
      0x04, 0x05, 0x06, 0x07, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01,
  };
  JoinRequest join_request1;
  JoinRequest join_request2;
  // Ensure that two members created with equal seed and do not
  // interfere each other. Member1 is deleted by the time member2
  // is created.
  {
    Prng prng;
    SetMemberParams(Prng::Generate, &prng, &f, &params);
    MemberCtxObj member1(&params);
    THROW_ON_EPIDERR(EpidMemberSetHashAlg(member1, kSha256));
    prng.set_seed(0x1234);
    EXPECT_EQ(kEpidNoErr,
              EpidCreateJoinRequest(member1, &pub_key, &ni, &join_request1));
    EXPECT_NO_FATAL_FAILURE(
        ValidateJoinRequest(join_request1, kSha256, pub_key, f, ni));
  }
  {
    Prng prng;
    SetMemberParams(Prng::Generate, &prng, &f, &params);
    MemberCtxObj member2(&params);
    THROW_ON_EPIDERR(EpidMemberSetHashAlg(member2, kSha512));
    prng.set_seed(0x1234);
    EXPECT_EQ(kEpidNoErr,
              EpidCreateJoinRequest(member2, &pub_key, &ni, &join_request2));
    EXPECT_NO_FATAL_FAILURE(
        ValidateJoinRequest(join_request2, kSha512, pub_key, f, ni));
  }
  EXPECT_NE(join_request1, join_request2);
}

TEST_F(EpidMemberTest,
       CreateJoinRequestWorksGivenValidParametersUsingIKGFData) {
  Prng prng;
  MemberParams params = {0};
  const GroupPubKey* pub_key = reinterpret_cast<const GroupPubKey*>(
      this->kGroupPublicKeyDataIkgf.data());
  FpElemStr f = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
  };
  IssuerNonce ni = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03,
      0x04, 0x05, 0x06, 0x07, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01,
  };
  JoinRequest join_request;
  SetMemberParams(Prng::Generate, &prng, &f, &params);
  MemberCtxObj member(&params);
  THROW_ON_EPIDERR(EpidMemberSetHashAlg(member, kSha512));
  EXPECT_EQ(kEpidNoErr,
            EpidCreateJoinRequest(member, pub_key, &ni, &join_request));
  EXPECT_NO_FATAL_FAILURE(
      ValidateJoinRequest(join_request, kSha512, *pub_key, f, ni));
}
}  // namespace
