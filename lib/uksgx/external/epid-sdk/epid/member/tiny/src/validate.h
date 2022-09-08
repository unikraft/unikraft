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
#ifndef EPID_MEMBER_TINY_SRC_VALIDATE_H_
#define EPID_MEMBER_TINY_SRC_VALIDATE_H_

/// \cond
typedef struct FpElem FpElem;

typedef struct GroupPubKey GroupPubKey;
typedef struct NativeGroupPubKey NativeGroupPubKey;

typedef struct PrivKey PrivKey;
typedef struct NativePrivKey NativePrivKey;

typedef struct MembershipCredential MembershipCredential;
typedef struct NativeMembershipCredential NativeMembershipCredential;

typedef struct PairingState PairingState;
/// \endcond

/// Test if the elements of a Group Public Key are in appropriate ranges.
/*!
\param[in] input the value to test.
\returns A value different from zero (i.e., true) if indeed
         all elements are in range. Zero (i.e., false) otherwise.
*/
int GroupPubKeyIsInRange(NativeGroupPubKey const* input);

/// Test if the elements of a Membership Credential are in appropriate ranges.
/*!
\param[in] input the value to test.
\returns A value different from zero (i.e., true) if indeed
         all elements are in range. Zero (i.e., false) otherwise.
*/
int MembershipCredentialIsInRange(NativeMembershipCredential const* input);

/// Test if a Membership Credential is in fact in the specified group.
/*!
\param[in] input the value to test.
\param[in] f the f value.
\param[in] pubkey the group key.
\param[in] pairing_state previously computed pairing state.
\returns A value different from zero (i.e., true) if indeed
         it is in the group. Zero (i.e., false) otherwise.
*/
int MembershipCredentialIsInGroup(NativeMembershipCredential const* input,
                                  FpElem const* f,
                                  NativeGroupPubKey const* pubkey,
                                  PairingState const* pairing_state);

/// Test if the elements of a Private Key are in appropriate ranges.
/*!
\param[in] input the value to test.
\returns A value different from zero (i.e., true) if indeed
         all elements are in range. Zero (i.e., false) otherwise.
*/
int PrivKeyIsInRange(NativePrivKey const* input);

/// Test if a Private Key is in fact in the specified group.
/*!
\param[in] input the value to test.
\param[in] pubkey the group key.
\param[in] pairing_state previously computed pairing state.
\returns A value different from zero (i.e., true) if indeed
         it is in the group. Zero (i.e., false) otherwise.
*/
int PrivKeyIsInGroup(NativePrivKey const* input,
                     NativeGroupPubKey const* pubkey,
                     PairingState const* pairing_state);

#endif  // EPID_MEMBER_TINY_SRC_VALIDATE_H_
