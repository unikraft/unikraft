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
/// EpidProvisionCompressed implementation.
/*!
 * \file
 */

#include <epid/member/api.h>

#include <string.h>
#include "epid/common/types.h"
#include "epid/member/src/context.h"

EpidStatus EpidProvisionCompressed(MemberCtx* ctx, GroupPubKey const* pub_key,
                                   CompressedPrivKey const* compressed_privkey,
                                   MemberPrecomp const* precomp_str) {
  EpidStatus sts = kEpidErr;
  PrivKey priv_key;
  if (!pub_key || !compressed_privkey || !ctx) {
    return kEpidBadArgErr;
  }
  sts = EpidDecompressPrivKey(pub_key, compressed_privkey, &priv_key);
  if (sts != kEpidNoErr) {
    return sts;
  }
  sts = EpidProvisionKey(ctx, pub_key, &priv_key, precomp_str);
  return sts;
}
