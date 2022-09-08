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
/// Basename hashing helper implementation
/*! \file */

#include "epid/member/src/hash_basename.h"

#include "epid/common/math/ecgroup.h"

/// Handle SDK Error with Break
#define BREAK_ON_EPID_ERROR(ret) \
  if (kEpidNoErr != (ret)) {     \
    break;                       \
  }

EpidStatus HashBaseName(EcGroup* G1, HashAlg hash_alg, void const* basename,
                        size_t basename_len, G1ElemStr* B_str,
                        uint32_t* iterations) {
  EpidStatus sts = kEpidErr;
  EcPoint* B = NULL;

  if (!G1 || (0 != basename_len && !basename) || !B_str) {
    return kEpidBadArgErr;
  }

  do {
    sts = NewEcPoint(G1, &B);
    BREAK_ON_EPID_ERROR(sts);

    sts = EcHash(G1, basename, basename_len, hash_alg, B, iterations);
    BREAK_ON_EPID_ERROR(sts);
    sts = WriteEcPoint(G1, B, B_str, sizeof(*B_str));
    BREAK_ON_EPID_ERROR(sts);

    sts = kEpidNoErr;
  } while (0);

  DeleteEcPoint(&B);

  return sts;
}
