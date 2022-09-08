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
/// Tpm2Commit implementation.
/*! \file */

#include "epid/member/tpm2/commit.h"
#include <tss2/TPM_Types.h>
#include <tss2/tss.h>
#include "epid/common/math/ecgroup.h"
#include "epid/common/src/epid2params.h"
#include "epid/common/src/memory.h"
#include "epid/member/tpm2/ibm_tss/conversion.h"
#include "epid/member/tpm2/ibm_tss/printtss.h"
#include "epid/member/tpm2/ibm_tss/state.h"

/// Handle Intel(R) EPID Error with Break
#define BREAK_ON_EPID_ERROR(ret) \
  if (kEpidNoErr != (ret)) {     \
    break;                       \
  }

/// Bit 7 binary mask
#define BIT7 0x080
/// Binary 00011111
#define BITS0500 0x3f

EpidStatus Tpm2Commit(Tpm2Ctx* ctx, EcPoint const* p1, void const* s2,
                      size_t s2_len, FfElement const* y2, EcPoint* k,
                      EcPoint* l, EcPoint* e, uint16_t* counter) {
  EpidStatus sts = kEpidErr;
  TPM_RC rc = TPM_RC_SUCCESS;

  if (!ctx || !ctx->epid2_params || !ctx->key_handle) {
    return kEpidBadArgErr;
  }

  if (s2 && s2_len <= 0) {
    return kEpidBadArgErr;
  }

  if ((!s2 && y2) || (s2 && !y2)) {
    return kEpidBadArgErr;
  }

  if (s2 && (!k || !l)) {
    return kEpidBadArgErr;
  }

  if (!e || !counter) {
    return kEpidBadArgErr;
  }

  if (s2_len > UINT16_MAX) {
    return kEpidBadArgErr;
  }

  do {
    FiniteField* Fq = ctx->epid2_params->Fq;
    EcGroup* G1 = ctx->epid2_params->G1;
    Commit_In in = {0};
    Commit_Out out;
    TPMI_SH_AUTH_SESSION sessionHandle0 = TPM_RS_PW;
    unsigned int sessionAttributes0 = 0;

    in.signHandle = ctx->key_handle;
    if (p1) {
      G1ElemStr p1_str = {0};
      sts = WriteEcPoint(G1, p1, &p1_str, sizeof(p1_str));
      BREAK_ON_EPID_ERROR(sts);
      sts = ReadTpm2EcPoint(&p1_str, &in.P1);
      BREAK_ON_EPID_ERROR(sts);
    }
    if (s2) {
      FqElemStr y2_str = {0};
      sts = WriteFfElement(Fq, y2, &y2_str, sizeof(y2_str));
      BREAK_ON_EPID_ERROR(sts);
      sts = ReadTpm2FfElement(&y2_str.data, &in.y2);
      BREAK_ON_EPID_ERROR(sts);
      in.s2.t.size = (UINT16)s2_len;
      if (0 != memcpy_S(&in.s2.t.buffer, sizeof(in.s2.t.buffer), s2, s2_len)) {
        sts = kEpidBadArgErr;
        break;
      }
    }
    rc = TSS_Execute(ctx->tss, (RESPONSE_PARAMETERS*)&out,
                     (COMMAND_PARAMETERS*)&in, NULL, TPM_CC_Commit,
                     sessionHandle0, NULL, sessionAttributes0, TPM_RH_NULL,
                     NULL, 0);
    if (rc != TPM_RC_SUCCESS) {
      print_tpm2_response_code("TPM2_Commit", rc);
      // workaround based on Table 2:15 to filter response code format defining
      // handle, session, or parameter number modifier if bit 7 is 1 error is
      // RC_FMT1
      if ((rc & BIT7) != 0) {
        rc = rc & (BITS0500 | RC_FMT1);
        if (TPM_RC_ATTRIBUTES == rc || TPM_RC_ECC_POINT == rc ||
            TPM_RC_HASH == rc || TPM_RC_KEY == rc || TPM_RC_SCHEME == rc ||
            TPM_RC_SIZE == rc)
          sts = kEpidBadArgErr;
        else
          sts = kEpidErr;
      } else {
        if (TPM_RC_NO_RESULT == rc)
          sts = kEpidBadArgErr;
        else
          sts = kEpidErr;
      }
      break;
    }
    if (out.E.size > 0) {
      G1ElemStr e_str = {0};
      sts = WriteTpm2EcPoint(&out.E, &e_str);
      BREAK_ON_EPID_ERROR(sts);
      sts = ReadEcPoint(G1, &e_str, sizeof(e_str), e);
      BREAK_ON_EPID_ERROR(sts);
    }
    if (out.K.size > 0 && k) {
      G1ElemStr k_str = {0};
      sts = WriteTpm2EcPoint(&out.K, &k_str);
      BREAK_ON_EPID_ERROR(sts);
      sts = ReadEcPoint(G1, &k_str, sizeof(k_str), k);
      BREAK_ON_EPID_ERROR(sts);
    }
    if (out.L.size > 0 && l) {
      G1ElemStr l_str = {0};
      sts = WriteTpm2EcPoint(&out.L, &l_str);
      BREAK_ON_EPID_ERROR(sts);
      sts = ReadEcPoint(G1, &l_str, sizeof(l_str), l);
      BREAK_ON_EPID_ERROR(sts);
    }
    *counter = out.counter;
  } while (0);
  return sts;
}
