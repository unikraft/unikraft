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
//        ippsGFpECPrivateKey()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"

/*F*
//    Name: ippsGFpECPrivateKey
//
// Purpose: Generate random private key
//
// Returns:                Reason:
//    ippStsNullPtrErr        NULL == pEC
//                            NULL == pPrivate
//
//    ippStsContextMatchErr   illegal pEC->idCtx
//                            pEC->subgroup == NULL
//                            illegal pPrivate->idCtx
//
//    ippStsSizeErr           BN_ROOM(pPrivate)*BITSIZE(BNU_CHUNK_T)<ECP_ORDBITSIZE(pEC)
//
//    ippStsNoErr             no errors
//
// Parameters:
//    pPrivate    pointer to the resultant private key
//    pEC         pointer to the EC context
//    rndFunc     Specified Random Generator
//    pRndParam   Pointer to the Random Generator context
//
*F*/
IPPFUN(IppStatus, ippsGFpECPrivateKey, (IppsBigNumState* pPrivate, IppsGFpECState* pEC,
                                        IppBitSupplier rndFunc, void* pRndParam))
{
   IPP_BAD_PTR2_RET(pEC, rndFunc);

   IPP_BADARG_RET(!VALID_ECP_ID(pEC), ippStsContextMatchErr);
   IPP_BADARG_RET(!ECP_SUBGROUP(pEC), ippStsContextMatchErr);

   /* test private key */
   IPP_BAD_PTR1_RET(pPrivate);
   IPP_BADARG_RET(!BN_VALID_ID(pPrivate), ippStsContextMatchErr);
   IPP_BADARG_RET((BN_ROOM(pPrivate)*BITSIZE(BNU_CHUNK_T)<ECP_ORDBITSIZE(pEC)), ippStsSizeErr);

   {
      /* generate random private key X:  0 < X < R */
      BNU_CHUNK_T* pOrder = MOD_MODULUS(ECP_MONT_R(pEC));
      int orderBitLen = ECP_ORDBITSIZE(pEC);
      int orderLen = BITS_BNU_CHUNK(orderBitLen);

      BNU_CHUNK_T* pX = BN_NUMBER(pPrivate);
      int nsX = BITS_BNU_CHUNK(orderBitLen);
      BNU_CHUNK_T xMask = MASK_BNU_CHUNK(orderBitLen);

      IppStatus sts;
      do {
         sts = rndFunc((Ipp32u*)pX, orderBitLen, pRndParam);
         if(ippStsNoErr!=sts)
            break;
         pX[nsX-1] &= xMask;
      } while( (1 == cpEqu_BNU_CHUNK(pX, nsX, 0)) ||
               (0 <= cpCmp_BNU(pX, nsX, pOrder, orderLen)) );

      /* set up private */
      if(ippStsNoErr==sts) {
         BN_SIGN(pPrivate) = ippBigNumPOS;
         FIX_BNU(pX, nsX);
         BN_SIZE(pPrivate) = nsX;
      }

      return sts;
   }
}
