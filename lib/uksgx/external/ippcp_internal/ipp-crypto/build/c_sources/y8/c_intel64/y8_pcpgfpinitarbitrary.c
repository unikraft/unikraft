/*******************************************************************************
* Copyright 2018-2021 Intel Corporation
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
//     Operations over GF(p).
// 
//     Context:
//        ippsGFpInitArbitrary()
// 
*/
#include "owndefs.h"
#include "owncp.h"

#include "pcpgfpstuff.h"
#include "pcpgfpxstuff.h"
#include "pcptool.h"


/*F*
// Name: ippsGFpInitArbitrary
//
// Purpose: initializes prime finite field GF(p)
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pPrime
//                               NULL == pGFp
//
//    ippStsSizeErr              !(IPP_MIN_GF_BITSIZE <= primeBitSize <=IPP_MAX_GF_BITSIZE)
//
//    ippStsContextMatchErr      incorrect pPrime context ID
//
//    ippStsBadArgErr            prime <0
//                               bitsize(prime) != primeBitSize
//                               prime <IPP_MIN_GF_CHAR
//                               prime is even
//
//    ippStsNoErr                no error
//
// Parameters:
//    pPrime         pointer to the prime context
//    primeBitSize   length of prime in bits
//    pGFp           pointer to Finite Field context is being initialized
*F*/
IPPFUN(IppStatus, ippsGFpInitArbitrary,(const IppsBigNumState* pPrime, int primeBitSize, IppsGFpState* pGFp))
{
   IPP_BAD_PTR1_RET(pGFp);

   IPP_BADARG_RET((primeBitSize< IPP_MIN_GF_BITSIZE) || (primeBitSize> IPP_MAX_GF_BITSIZE), ippStsSizeErr);

   IPP_BAD_PTR1_RET(pPrime);
   IPP_BADARG_RET(!BN_VALID_ID(pPrime), ippStsContextMatchErr);
   IPP_BADARG_RET(BN_SIGN(pPrime)!= IppsBigNumPOS, ippStsBadArgErr);                                   /* prime is negative */
   IPP_BADARG_RET(BITSIZE_BNU(BN_NUMBER(pPrime),BN_SIZE(pPrime)) != primeBitSize, ippStsBadArgErr);  /* primeBitSize == bitsize(prime) */
   IPP_BADARG_RET((BN_SIZE(pPrime)==1) && (BN_NUMBER(pPrime)[0]<IPP_MIN_GF_CHAR), ippStsBadArgErr);  /* prime < 3 */
   IPP_BADARG_RET(0==(BN_NUMBER(pPrime)[0] & 1), ippStsBadArgErr);                                     /* prime is even */

   {
      /* init GF */
      IppStatus sts = cpGFpInitGFp(primeBitSize, pGFp);

      /* set up GF engine */
      if(ippStsNoErr==sts) {
         cpGFpSetGFp(BN_NUMBER(pPrime), primeBitSize, ippsGFpMethod_pArb(), pGFp);
      }

      return sts;
   }
}
