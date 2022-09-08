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
//        ippsGFpInitFixed()
// 
*/
#include "owndefs.h"
#include "owncp.h"

#include "pcpgfpstuff.h"
#include "pcpgfpxstuff.h"
#include "pcptool.h"


/*F*
// Name: ippsGFpInitFixed
//
// Purpose: initializes prime finite field GF(p)
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pGFpMethod
//                               NULL == pGFp
//
//    ippStsBadArgErr            pGFpMethod != ippsGFpMethod_pXXX() any fixed prime method
//                               primeBitSize != sizeof modulus defined by fixed method
//
//    ippStsNoErr                no error
//
// Parameters:
//    primeBitSize   length of prime in bits
//    pGFpMethod     pointer to the basic arithmetic metods
//    pGFp           pointer to Finite Field context is being initialized
*F*/
IPPFUN(IppStatus, ippsGFpInitFixed,(int primeBitSize, const IppsGFpMethod* pGFpMethod, IppsGFpState* pGFp))
{
   IPP_BAD_PTR2_RET(pGFpMethod, pGFp);

   /* test method is prime based */
   IPP_BADARG_RET(cpID_Prime!=(pGFpMethod->modulusID & cpID_Prime), ippStsBadArgErr);
   /* test if method is not prime based arbitrary */
   IPP_BADARG_RET(!pGFpMethod->modulus, ippStsBadArgErr);
   /* size of the underlying prime must be equal to primeBitSize parameter*/
   IPP_BADARG_RET(pGFpMethod->modulusBitDeg!=primeBitSize, ippStsBadArgErr);

   {
      /* init GF */
      IppStatus sts = cpGFpInitGFp(primeBitSize, pGFp);

      /* set up GF engine */
      if(ippStsNoErr==sts) {
         cpGFpSetGFp(pGFpMethod->modulus, primeBitSize, pGFpMethod, pGFp);
      }

      return sts;
   }
}
