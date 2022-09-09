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
//        pcpgfpxinitbinomial.c()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpstuff.h"
#include "pcpgfpxstuff.h"
#include "pcptool.h"


/*F*
// Name: ippsGFpxInitBinomial
//
// Purpose: initializes finite field extension GF(p^d)
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pGFpx
//                               NULL == pGroundGF
//                               NULL == pGroundElm
//                               NULL == pGFpMethod
//
//    ippStsContextMatchErr      incorrect pGroundGF's context ID
//                               incorrect pGroundElm's context ID
//
//    ippStsOutOfRangeErr        size of pGroundElm does not equal to size of pGroundGF element
//
//    ippStsBadArgErr            IPP_MIN_GF_EXTDEG > extDeg || extDeg > IPP_MAX_GF_EXTDEG
//                                  (IPP_MIN_GF_EXTDEG==2, IPP_MAX_GF_EXTDEG==8)
//
//                               cpID_Poly!=pGFpMethod->modulusID  -- method does not refferenced to polynomial one
//                               pGFpMethod->modulusBitDeg!=extDeg -- fixed method does not match to degree extension
//
//    ippStsNoErr                no error
//
// Parameters:
//    pGroundGF      pointer to the context of the finite field is being extension
//    extDeg         degree of extension
//    pGroundElm     pointer to the IppsGFpElement context containing the trailing coefficient of the field binomial.
//    pGFpMethod     pointer to the basic arithmetic metods
//    pGFpx          pointer to Finite Field context is being initialized
*F*/
IPPFUN(IppStatus, ippsGFpxInitBinomial,(const IppsGFpState* pGroundGF, int extDeg,
                                        const IppsGFpElement* pGroundElm,
                                        const IppsGFpMethod* pGFpMethod,
                                        IppsGFpState* pGFpx))
{
   IPP_BAD_PTR4_RET(pGFpx, pGroundGF, pGroundElm, pGFpMethod);

   IPP_BADARG_RET( !GFP_VALID_ID(pGroundGF), ippStsContextMatchErr );

   IPP_BADARG_RET( !GFPE_VALID_ID(pGroundElm), ippStsContextMatchErr );
   IPP_BADARG_RET(GFPE_ROOM(pGroundElm)!=GFP_FELEN(GFP_PMA(pGroundGF)), ippStsOutOfRangeErr);

   IPP_BADARG_RET( extDeg<IPP_MIN_GF_EXTDEG || extDeg>IPP_MAX_GF_EXTDEG, ippStsBadArgErr);

   /* test method is binomial based */
   IPP_BADARG_RET(cpID_Binom != (pGFpMethod->modulusID & cpID_Binom), ippStsBadArgErr);

   /* test if method assums fixed degree extension */
   IPP_BADARG_RET(pGFpMethod->modulusBitDeg && (extDeg!=pGFpMethod->modulusBitDeg), ippStsBadArgErr);

   /* init context */
   InitGFpxCtx(pGroundGF, extDeg, pGFpMethod, pGFpx);

   /* store low-order coefficient of irresucible into the context */
   cpGFpElementCopy(GFP_MODULUS(GFP_PMA(pGFpx)), GFPE_DATA(pGroundElm), GFP_FELEN(GFP_PMA(pGroundGF)));

   return ippStsNoErr;
}
