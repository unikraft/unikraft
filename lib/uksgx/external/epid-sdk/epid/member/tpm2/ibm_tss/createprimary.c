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

/*!
 * \brief TPM2_CreatePrimary command implementation.
 * \file
 */
#include "epid/member/tpm2/createprimary.h"
#include "epid/common/src/memory.h"
#include "epid/member/tpm2/ibm_tss/conversion.h"
#include "epid/member/tpm2/ibm_tss/printtss.h"
#include "epid/member/tpm2/ibm_tss/state.h"
#include "tss2/TPM_Types.h"
#include "tss2/tss.h"

EpidStatus Tpm2CreatePrimary(Tpm2Ctx* ctx, G1ElemStr* p_str) {
  if (!ctx || !ctx->epid2_params || !p_str) {
    return kEpidBadArgErr;
  }
  CreatePrimary_In in = {0};
  CreatePrimary_Out out;
  TPM_RC rc = TPM_RC_SUCCESS;
  TPMI_ALG_PUBLIC algPublic = TPM_ALG_ECC;
  TPMI_ECC_CURVE curveID = TPM_ECC_BN_P256;
  TPMI_ALG_HASH halg = TPM_ALG_NULL;
  TPMI_ALG_HASH nalg = TPM_ALG_NULL;
  TPMI_SH_AUTH_SESSION sessionHandle0 = TPM_RS_PW;
  TPM2B_ECC_POINT public_area;
  unsigned int sessionAttributes0 = 0;
  const char* parentPasswordPtr = NULL;

  in.primaryHandle = TPM_RH_ENDORSEMENT;
  halg = EpidtoTpm2HashAlg(ctx->hash_alg);
  if (halg == TPM_ALG_NULL) {
    return kEpidHashAlgorithmNotSupported;
  }
  nalg = halg;
  /* Table 185 - TPM2B_PUBLIC inPublic */
  /* Table 184 - TPMT_PUBLIC in.inPublic.publicArea */
  in.inPublic.publicArea.type = algPublic;
  in.inPublic.publicArea.nameAlg = nalg;

  /* Table 32 - TPMA_OBJECT objectAttributes */
  in.inPublic.publicArea.objectAttributes.val |= TPMA_OBJECT_NODA;
  in.inPublic.publicArea.objectAttributes.val |= TPMA_OBJECT_FIXEDTPM;
  in.inPublic.publicArea.objectAttributes.val |= TPMA_OBJECT_FIXEDPARENT;
  in.inPublic.publicArea.objectAttributes.val |=
      TPMA_OBJECT_SENSITIVEDATAORIGIN;
  in.inPublic.publicArea.parameters.eccDetail.symmetric.algorithm =
      TPM_ALG_NULL;
  in.inPublic.publicArea.parameters.eccDetail.scheme.scheme = TPM_ALG_ECDAA;
  in.inPublic.publicArea.parameters.eccDetail.scheme.details.ecdaa.hashAlg =
      halg;
  in.inPublic.publicArea.parameters.eccDetail.scheme.details.ecdaa.count = 1;
  in.inPublic.publicArea.parameters.eccDetail.curveID = curveID;
  in.inPublic.publicArea.parameters.eccDetail.kdf.scheme = TPM_ALG_NULL;
  in.inSensitive.sensitive.userAuth.t.size = 0;
  in.inSensitive.sensitive.data.t.size = 0;
  in.inPublic.publicArea.objectAttributes.val |=
      TPMA_OBJECT_SENSITIVEDATAORIGIN;
  in.inPublic.publicArea.objectAttributes.val |= TPMA_OBJECT_USERWITHAUTH;
  in.inPublic.publicArea.objectAttributes.val &= ~TPMA_OBJECT_ADMINWITHPOLICY;
  in.inPublic.publicArea.objectAttributes.val |= TPMA_OBJECT_SIGN;
  in.inPublic.publicArea.objectAttributes.val &= ~TPMA_OBJECT_DECRYPT;
  in.inPublic.publicArea.objectAttributes.val &= ~TPMA_OBJECT_RESTRICTED;
  in.inPublic.publicArea.unique.ecc.y.t.size = 0;
  in.inPublic.publicArea.unique.ecc.x.t.size = 0;
  in.inPublic.publicArea.authPolicy.t.size = 0;
  in.inPublic.publicArea.unique.rsa.t.size = 0;
  in.outsideInfo.t.size = 0;
  in.creationPCR.count = 0;
  rc = TSS_Execute(ctx->tss, (RESPONSE_PARAMETERS*)&out,
                   (COMMAND_PARAMETERS*)&in, NULL, TPM_CC_CreatePrimary,
                   sessionHandle0, parentPasswordPtr, sessionAttributes0,
                   TPM_RH_NULL, NULL, 0);
  if (rc != TPM_RC_SUCCESS) {
    print_tpm2_response_code("TPM2_CreatePrimary", rc);
    if (TPM_RC_ATTRIBUTES == rc || TPM_RC_KDF == rc || TPM_RC_SYMMETRIC == rc ||
        TPM_RC_TYPE == rc || TPM_RC_SCHEME == rc || TPM_RC_SIZE == rc ||
        TPM_RC_KEY == rc)
      return kEpidBadArgErr;
    return kEpidErr;
  }
  ctx->key_handle = out.objectHandle;
  public_area.point = out.outPublic.publicArea.unique.ecc;
  return WriteTpm2EcPoint(&public_area, p_str);
}
