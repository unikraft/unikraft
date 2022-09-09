/*******************************************************************************
* Copyright 2010-2021 Intel Corporation
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
//     Operations over GF(p) ectension.
// 
//     Context:
//        pcpgfpxinit.c()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpstuff.h"
#include "pcpgfpxstuff.h"
#include "pcptool.h"



/*F*
// Name: ippsGFpxInit
//
// Purpose: initializes finite field extension GF(p^d)
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pGFpx
//                               NULL == pGroundGF
//                               NULL == ppGroundElm
//                               NULL == pGFpMethod
//
//    ippStsContextMatchErr      incorrect pGroundGF's context ID
//                               incorrect ppGroundElm[i]'s context ID
//
//    ippStsOutOfRangeErr        size of ppGroundElm[i] does not equal to size of pGroundGF element
//
//    ippStsBadArgErr            IPP_MIN_GF_EXTDEG > extDeg || extDeg > IPP_MAX_GF_EXTDEG
//                                  (IPP_MIN_GF_EXTDEG==2, IPP_MAX_GF_EXTDEG==8)
//                               1>nElm || nElm>extDeg
//
//                               cpID_Poly!=pGFpMethod->modulusID  -- method does not refferenced to polynomial one
//                               pGFpMethod->modulusBitDeg!=extDeg -- fixed method does not match to degree extension
//
//    ippStsNoErr                no error
//
// Parameters:
//    pGroundGF      pointer to the context of the finite field is being extension
//    extDeg         degree  of extension
//    ppGroundElm[]  pointer to the array of extension field polynomial
//    nElm           number  of coefficients above
//    pGFpMethod     pointer to the basic arithmetic metods
//    pGFpx          pointer to Finite Field context is being initialized
*F*/
IPPFUN(IppStatus, ippsGFpxInit,(const IppsGFpState* pGroundGF, int extDeg,
                                const IppsGFpElement* const ppGroundElm[], int nElm,
                                const IppsGFpMethod* pGFpMethod, IppsGFpState* pGFpx))
{
   IPP_BAD_PTR4_RET(pGFpx, pGroundGF, ppGroundElm, pGFpMethod);

   IPP_BADARG_RET( !GFP_VALID_ID(pGroundGF), ippStsContextMatchErr );

   /* test extension degree */
   IPP_BADARG_RET( extDeg<IPP_MIN_GF_EXTDEG || extDeg>IPP_MAX_GF_EXTDEG, ippStsBadArgErr);
   /* coeffs at (x^0), (x^1), ..., (x^(deg-1)) passed acually */
   /* considering normilized f(x), the coeff at (x^deg) is 1 and so could neither stored no passed */
   /* test if 1<=nElm<=extDeg */
   IPP_BADARG_RET( 1>nElm || nElm>extDeg, ippStsBadArgErr);

   /* test if method is polynomial based */
   IPP_BADARG_RET(cpID_Poly != (pGFpMethod->modulusID & cpID_Poly), ippStsBadArgErr);
   /* test if method is fixed polynomial based */
   IPP_BADARG_RET(pGFpMethod->modulusBitDeg && (pGFpMethod->modulusBitDeg!=extDeg), ippStsBadArgErr);

   InitGFpxCtx(pGroundGF, extDeg, pGFpMethod, pGFpx);

   {
      BNU_CHUNK_T* pPoly = GFP_MODULUS(GFP_PMA(pGFpx));
      int polyTermlen = GFP_FELEN(GFP_PMA(pGroundGF));
      int n;
      for(n=0; n<nElm; n++, pPoly+=polyTermlen) {
         const IppsGFpElement* pGroundElm = ppGroundElm[n];

         /* test element */
         IPP_BAD_PTR1_RET(pGroundElm);
         IPP_BADARG_RET(!GFPE_VALID_ID(pGroundElm), ippStsContextMatchErr);
         IPP_BADARG_RET(GFPE_ROOM(pGroundElm)!=polyTermlen, ippStsOutOfRangeErr);

         /* copy element */
         cpGFpElementCopy(pPoly, GFPE_DATA(pGroundElm), polyTermlen);
      }
   }

   return ippStsNoErr;
}
