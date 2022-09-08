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
/// Intel(R) EPID SDK member API.
/*! \file */
#ifndef EPID_MEMBER_API_H_
#define EPID_MEMBER_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "epid/common/bitsupplier.h"
#include "epid/common/epiddefs.h"
#include "epid/common/errors.h"
#include "epid/common/types.h"

/// Internal context of member.
typedef struct MemberCtx MemberCtx;

/// Implementation specific configuration parameters.
typedef struct MemberParams MemberParams;

/// Member functionality
/*!
  \defgroup EpidMemberModule member

  Defines the APIs needed by Intel(R) EPID members. Each member
  context (::MemberCtx) represents membership in a single group.

  To use this module, include the header epid/member/api.h.

  \ingroup EpidModule
  @{
*/

/// Allocates and initializes a new member context.
/*!
 \param[in] params
 Implementation specific configuration parameters.
 \param[out] ctx
 Newly constructed member context.

 \warning buffers allocated using this function should not be
 initialized with ::EpidMemberInit.

 \deprecated This API has been superseded by ::EpidMemberGetSize
 and ::EpidMemberInit.

 \returns ::EpidStatus
 */
EpidStatus EPID_API EpidMemberCreate(MemberParams const* params,
                                     MemberCtx** ctx);

/// Computes the size in bytes required for a member context
/*!
 \param[in] params
 Implementation specific configuration parameters.
 \param[out] context_size
 Number of bytes required for a ::MemberCtx buffer

 \returns ::EpidStatus
  \see EpidMemberInit
 */
EpidStatus EPID_API EpidMemberGetSize(MemberParams const* params,
                                      size_t* context_size);

/// Initializes a new member context.
/*!
 \param[in] params
 Implementation specific configuration parameters.
 \param[in,out] ctx
 An existing buffer that will be used as a ::MemberCtx.

 \warning ctx must be a buffer of at least the size reported by
 ::EpidMemberGetSize for the same parameters.

 \returns ::EpidStatus
 \see EpidMemberGetSize
 */
EpidStatus EPID_API EpidMemberInit(MemberParams const* params, MemberCtx* ctx);

/// Creates a request to join a group.
/*!
The created request is part of the interaction with an issuer needed to join
a group. This interaction with the issuer is outside the scope of this API.

\param[in,out] ctx
The member context.
\param[in] pub_key
The group certificate of group to join.
\param[in] ni
The nonce chosen by issuer as part of join protocol.
\param[out] join_request
The join request.

\returns ::EpidStatus
*/
EpidStatus EPID_API EpidCreateJoinRequest(MemberCtx* ctx,
                                          GroupPubKey const* pub_key,
                                          IssuerNonce const* ni,
                                          JoinRequest* join_request);

/// Provisions a member context from a membership credential
/*!
\param[in,out] ctx
The member context.
\param[in] pub_key
The group certificate of group to provision.
\param[in] credential
membership credential.
\param[in] precomp_str
Precomputed state (implementation specific optional)

\returns ::EpidStatus
*/
EpidStatus EPID_API EpidProvisionCredential(
    MemberCtx* ctx, GroupPubKey const* pub_key,
    MembershipCredential const* credential, MemberPrecomp const* precomp_str);

/// Provisions a member context from a compressed private key
/*!
\param[in,out] ctx
The member context.
\param[in] pub_key
The group certificate of group to provision.
\param[in] compressed_privkey
private key.
\param[in] precomp_str
Precomputed state (implementation specific optional)

\returns ::EpidStatus
*/
EpidStatus EPID_API
EpidProvisionCompressed(MemberCtx* ctx, GroupPubKey const* pub_key,
                        CompressedPrivKey const* compressed_privkey,
                        MemberPrecomp const* precomp_str);

/// Provisions a member context from a private key
/*!
\param[in,out] ctx
The member context.
\param[in] pub_key
The group certificate of group to provision.
\param[in] priv_key
private key.
\param[in] precomp_str
Precomputed state (implementation specific optional)

\returns ::EpidStatus
*/
EpidStatus EPID_API EpidProvisionKey(MemberCtx* ctx, GroupPubKey const* pub_key,
                                     PrivKey const* priv_key,
                                     MemberPrecomp const* precomp_str);

/// Change member from setup state to normal operation
/*!
\param[in,out] ctx
The member context.

\returns ::EpidStatus
*/
EpidStatus EPID_API EpidMemberStartup(MemberCtx* ctx);

/// De-initializes an existing member context buffer.
/*!
 Must be called to safely release a member context initialized using
 ::EpidMemberInit.

 De-initializes the context.

 \param[in,out] ctx
 The member context. Can be NULL.

 \warning This function should not be used on buffers allocated with
 ::EpidMemberCreate. Those buffers should be released using ::EpidMemberDelete

 \see EpidMemberInit
 */
void EPID_API EpidMemberDeinit(MemberCtx* ctx);

/// Deletes an existing member context.
/*!
 Must be called to safely release a member context created using
 ::EpidMemberCreate.

 De-initializes the context, frees memory used by the context, and sets the
 context pointer to NULL.

 \param[in,out] ctx
 The member context. Can be NULL.

 \deprecated This API has been superseded by ::EpidMemberDeinit.

 \see EpidMemberCreate

 \b Example

 \ref UserManual_GeneratingAnIntelEpidSignature
 */
void EPID_API EpidMemberDelete(MemberCtx** ctx);

/// Sets the hash algorithm to be used by a member.
/*!
 \param[in] ctx
 The member context.
 \param[in] hash_alg
 The hash algorithm to use.

 \returns ::EpidStatus

 \note
 If the result is not ::kEpidNoErr, the hash algorithm used by the member is
 undefined.

 \see EpidMemberInit
 \see ::HashAlg

 \b Example

 \ref UserManual_GeneratingAnIntelEpidSignature
 */
EpidStatus EPID_API EpidMemberSetHashAlg(MemberCtx* ctx, HashAlg hash_alg);

/// Sets the signature based revocation list to be used by a member.
/*!
 The caller is responsible for ensuring the revocation list is authorized,
 e.g. signed by the issuer. The caller is also responsible checking the version
 of the revocation list. The call fails if trying to set an older version
 of the revocation list than was last set.

 \attention
 The memory pointed to by sig_rl is accessed directly by the member
 until a new list is set or the member is destroyed. Do not modify the
 contents of this memory. The behavior of subsequent operations that rely on
 the revocation list is undefined if the memory is modified.

 \attention
 It is the responsibility of the caller to free the memory pointed to by sig_rl
 after the member is no longer using it.

 \param[in] ctx
 The member context.
 \param[in] sig_rl
 The signature based revocation list.
 \param[in] sig_rl_size
 The size of the signature based revocation list in bytes.

 \returns ::EpidStatus

 \note
 If the result is not ::kEpidNoErr the signature based revocation list pointed
 to by the member is not changed.

 \see EpidMemberInit

 \b Example

 \ref UserManual_GeneratingAnIntelEpidSignature
 */
EpidStatus EPID_API EpidMemberSetSigRl(MemberCtx* ctx, SigRl const* sig_rl,
                                       size_t sig_rl_size);

/// Computes the size in bytes required for an Intel(R) EPID signature.
/*!
 \param[in] sig_rl
 The signature based revocation list that is used. NULL is treated as
 a zero length list.

 \returns
 Size in bytes of an Intel(R) EPID signature including proofs for each entry
 in the signature based revocation list.

 \see ::SigRl

 \b Example

 \ref UserManual_GeneratingAnIntelEpidSignature
*/
size_t EPID_API EpidGetSigSize(SigRl const* sig_rl);

/// Writes an Intel(R) EPID signature.
/*!
 \param[in] ctx
 The member context.
 \param[in] msg
 The message to sign.
 \param[in] msg_len
 The length in bytes of message.
 \param[in] basename
 Optional basename. If basename is NULL a random basename is used.
 Signatures generated using random basenames are anonymous. Signatures
 generated using the same basename are linkable by the verifier. If a
 basename is provided, it must already be registered, or
 ::kEpidBadArgErr is returned.
 \param[in] basename_len
 The size of basename in bytes. Must be 0 if basename is NULL.
 \param[out] sig
 The generated signature
 \param[in] sig_len
 The size of signature in bytes. Must be equal to value returned by
 EpidGetSigSize().

 \returns ::EpidStatus

 \note
 If the result is not ::kEpidNoErr the content of sig is undefined.

 \see EpidMemberInit
 \see EpidMemberSetHashAlg
 \see EpidMemberSetSigRl
 \see EpidGetSigSize

 \b Example

 \ref UserManual_GeneratingAnIntelEpidSignature
 */
EpidStatus EPID_API EpidSign(MemberCtx const* ctx, void const* msg,
                             size_t msg_len, void const* basename,
                             size_t basename_len, EpidSignature* sig,
                             size_t sig_len);

/// Registers a basename with a member.
/*!

 To prevent loss of privacy, the member keeps a list of basenames
 (corresponding to authorized verifiers). The member signs a message
 with a basename only if the basename is in the member's basename
 list.

 \warning
 The use of a name-based signature creates a platform unique
 pseudonymous identifier. Because it reduces the member's privacy, the
 user should be notified when it is used and should have control over
 its use.

 \param[in] ctx
 The member context.
 \param[in] basename
 The basename.
 \param[in] basename_len
 Length of the basename.

 \returns ::EpidStatus

 \retval ::kEpidDuplicateErr
 The basename was already registered.

 \note
 If the result is not ::kEpidNoErr or ::kEpidDuplicateErr it is undefined if the
 basename is registered.

 \b Example

 \ref UserManual_GeneratingAnIntelEpidSignature
 */
EpidStatus EPID_API EpidRegisterBasename(MemberCtx* ctx, void const* basename,
                                         size_t basename_len);

/// Clears registered basenames.
/*!

 Allows clearing registered basenames without recreating member.

 \param[in,out] ctx
 The member context.

 \returns ::EpidStatus

 \see ::EpidRegisterBasename
 */
EpidStatus EPID_API EpidClearRegisteredBasenames(MemberCtx* ctx);

/// Extends the member's pool of pre-computed signatures.
/*!
  Generate new pre-computed signatures and add them to the internal pool.

 \param[in] ctx
 The member context.
 \param[in] number_presigs
 The number of pre-computed signatures to add to the internal pool.

 \returns ::EpidStatus

 \see ::EpidMemberInit
 */
EpidStatus EPID_API EpidAddPreSigs(MemberCtx* ctx, size_t number_presigs);

/// Gets the number of pre-computed signatures in the member's pool.
/*!
 \param[in] ctx
 The member context.

 \returns
 Number of remaining pre-computed signatures. Returns 0 if ctx is NULL.

 \see ::EpidMemberInit
*/
size_t EPID_API EpidGetNumPreSigs(MemberCtx const* ctx);

/// Decompresses compressed member private key.
/*!

  Converts a compressed member private key into a member
  private key for use by other member APIs.

  \param[in] pub_key
  The public key of the group.
  \param[in] compressed_privkey
  The compressed member private key to be decompressed.
  \param[out] priv_key
  The member private key.

  \returns ::EpidStatus

  \b Example

  \ref UserManual_GeneratingAnIntelEpidSignature
 */
EpidStatus EPID_API EpidDecompressPrivKey(
    GroupPubKey const* pub_key, CompressedPrivKey const* compressed_privkey,
    PrivKey* priv_key);

/*! @} */

#ifdef __cplusplus
}
#endif

#endif  // EPID_MEMBER_API_H_
