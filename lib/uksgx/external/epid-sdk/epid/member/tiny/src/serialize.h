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
/// Definition of de/serialize functionality.

#ifndef EPID_MEMBER_TINY_SRC_SERIALIZE_H_
#define EPID_MEMBER_TINY_SRC_SERIALIZE_H_

/// \cond
typedef struct BasicSignature BasicSignature;
typedef struct NativeBasicSignature NativeBasicSignature;

typedef struct GroupPubKey GroupPubKey;
typedef struct NativeGroupPubKey NativeGroupPubKey;

typedef struct PrivKey PrivKey;
typedef struct NativePrivKey NativePrivKey;

typedef struct MembershipCredential MembershipCredential;
typedef struct NativeMembershipCredential NativeMembershipCredential;

typedef struct MemberPrecomp MemberPrecomp;
typedef struct NativeMemberPrecomp NativeMemberPrecomp;
/// \endcond

/// Write a basic signature to a portable buffer
/*!
\param[out] dest target buffer
\param [in] src source data
\result pointer to next byte after final data written to dest
*/
void* BasicSignatureSerialize(BasicSignature* dest,
                              NativeBasicSignature const* src);

/// Read a basic signature from a portable buffer
/*!
\param[out] dest target buffer
\param [in] src source data
\result pointer to next byte after final data read from to src
*/
void const* BasicSignatureDeserialize(NativeBasicSignature* dest,
                                      BasicSignature const* src);

/// Write a group public key to a portable buffer
/*!
\param[out] dest target buffer
\param [in] src source data
\result pointer to next byte after final data written to dest
*/
void* GroupPubKeySerialize(GroupPubKey* dest, NativeGroupPubKey const* src);

/// Read a group public key from a portable buffer
/*!
\param[out] dest target buffer
\param [in] src source data
\result pointer to next byte after final data read from to src
*/
void const* GroupPubKeyDeserialize(NativeGroupPubKey* dest,
                                   GroupPubKey const* src);

/// Read a member private key from a portable buffer
/*!
\param[out] dest target buffer
\param [in] src source data
\result pointer to next byte after final data read from to src
*/
void const* PrivKeyDeserialize(NativePrivKey* dest, PrivKey const* src);

/// Read pairing pre-computation data from a portable buffer
/*!
\param[out] dest target buffer
\param [in] src source data
\result pointer to next byte after final data read from to src
*/
void const* PreCompDeserialize(NativeMemberPrecomp* dest,
                               MemberPrecomp const* src);

/// Read a membership credential from a portable buffer
/*!
\param[out] dest target buffer
\param [in] src source data
\result pointer to next byte after final data read from to src
*/
void const* MembershipCredentialDeserialize(NativeMembershipCredential* dest,
                                            MembershipCredential const* src);
#endif  // EPID_MEMBER_TINY_SRC_SERIALIZE_H_
