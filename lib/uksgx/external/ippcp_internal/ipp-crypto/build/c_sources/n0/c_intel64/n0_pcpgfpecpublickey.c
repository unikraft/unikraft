/*******************************************************************************
* Copyright 2016-2021 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

/*
//     Intel(R) Integrated Performance Primitives. Cryptography Primitives.
// 
//     Context:
//        ippsGFpECPublicKey()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"



/*F*
//    Name: ippsGFpECPublicKey
//
// Purpose: Compute Public Key
//
// Returns:                Reason:
//    ippStsNullPtrErr        NULL == pEC
//                            NULL == pPrivate
//                            NULL == pPublic
//                            NULL == pScratchBuffer
//
//    ippStsContextMatchErr   illegal pEC->idCtx
//                            pEC->subgroup == NULL
//                            illegal pPrivate->idCtx
//                            illegal pPublic->idCtx
//
//    ippStsIvalidPrivateKey  !(0 < pPrivate < order)
//
//    ippStsRangeErr          ECP_POINT_FELEN(pPublic)<GFP_FELEN()
//
//    ippStsNoErr             no errors
//
// Parameters:
//    pPrivate       pointer to the private key
//    pPublic        pointer to the resultant public key
//    pEC            pointer to the EC context
//    pScratchBuffer pointer to buffer (1 mul_point operation)
//
*F*/
IPPFUN(IppStatus, ippsGFpECPublicKey, (const IppsBigNumState* pPrivate, IppsGFpECPoint* pPublic,
                                      IppsGFpECState* pEC, Ipp8u* pScratchBuffer))
{
   /* EC context and buffer */
   IPP_BAD_PTR2_RET(pEC, pScratchBuffer);
   IPP_BADARG_RET(!VALID_ECP_ID(pEC), ippStsContextMatchErr);
   IPP_BADARG_RET(!ECP_SUBGROUP(pEC), ippStsContextMatchErr);

   /* test private keys */
   IPP_BAD_PTR1_RET(pPrivate);
   IPP_BADARG_RET(!BN_VALID_ID(pPrivate), ippStsContextMatchErr);

   /* test public key */
   IPP_BAD_PTR1_RET(pPublic);
   IPP_BADARG_RET(!ECP_POINT_VALID_ID(pPublic), ippStsContextMatchErr);
   IPP_BADARG_RET(ECP_POINT_FELEN(pPublic)<GFP_FELEN(GFP_PMA(ECP_GFP(pEC))), ippStsRangeErr);

   {
      BNU_CHUNK_T* pS= BN_NUMBER(pPrivate);
      int nsS= BN_SIZE(pPrivate);

      BNU_CHUNK_T* pOrder = MOD_MODULUS(ECP_MONT_R(pEC));
      int orderLen = BITS_BNU_CHUNK(ECP_ORDBITSIZE(pEC));

      IPP_BADARG_RET(cpEqu_BNU_CHUNK(pS, nsS, 0)
               || 0<=cpCmp_BNU(pS, nsS, pOrder, orderLen), ippStsIvalidPrivateKey);

      /* calculates public key */
      gfec_MulBasePoint(pPublic, pS, nsS, pEC, pScratchBuffer);

      return ippStsNoErr;
   }
}
