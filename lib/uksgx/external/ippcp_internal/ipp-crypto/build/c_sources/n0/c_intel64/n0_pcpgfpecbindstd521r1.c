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
//     EC over GF(p^m) definitinons
// 
//     Context:
//        ippsGFpECBindGxyTblStd521r1()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"
#include "pcpeccp.h"



static IppStatus cpGFpECBindGxyTbl(const BNU_CHUNK_T* pPrime,
                                   const cpPrecompAP* preComp,
                                   IppsGFpECState* pEC)
{
   IPP_BAD_PTR1_RET(pEC);
   IPP_BADARG_RET(!VALID_ECP_ID(pEC), ippStsContextMatchErr);

   {
      IppsGFpState* pGF = ECP_GFP(pEC);
      gsModEngine* pGFE = GFP_PMA(pGF);
      Ipp32u elemLen = (Ipp32u)GFP_FELEN(pGFE);

      /* test if GF is prime GF */
      IPP_BADARG_RET(!GFP_IS_BASIC(pGFE), ippStsBadArgErr);
      /* test underlying prime value*/
      IPP_BADARG_RET(cpCmp_BNU(pPrime, (cpSize)elemLen, GFP_MODULUS(pGFE), (cpSize)elemLen), ippStsBadArgErr);

      {
         BNU_CHUNK_T* pbp_ec = ECP_G(pEC);
         int cmpFlag;
         BNU_CHUNK_T* pbp_tbl = cpEcGFpGetPool(1, pEC);

         selectAP select_affine_point = preComp->select_affine_point;
         const BNU_CHUNK_T* pTbl = preComp->pTbl;
         select_affine_point(pbp_tbl, pTbl, 1);

         /* check if EC's and G-table's Base Point is the same */
         cmpFlag = cpCmp_BNU(pbp_ec, (cpSize)elemLen*2, pbp_tbl, (cpSize)elemLen*2);

         cpEcGFpReleasePool(1, pEC);

         return cmpFlag? ippStsBadArgErr : ippStsNoErr;
      }
   }
}

/*F*
// Name: ippsGFpECBindGxyTblStd521r1
//
// Purpose: Enables the use of base point-based pre-computed tables of EC521r1
//
// Returns:                   Reason:
//    ippStsNullPtrErr              NULL == pEC
//
//    ippStsContextMatchErr         invalid pEC->idCtx
//
//    ippStsBadArgErr               pEC is not EC521r1
//
//    ippStsNoErr                   no error
//
// Parameters:
//    pEC        Pointer to the context of the elliptic curve
//
*F*/

IPPFUN(IppStatus, ippsGFpECBindGxyTblStd521r1,(IppsGFpECState* pEC))
{
   IppStatus sts = cpGFpECBindGxyTbl(secp521r1_p, gfpec_precom_nistP521r1_fun(), pEC);

   /* setup pre-computed g-table and point access function */
   if(ippStsNoErr==sts)
      ECP_PREMULBP(pEC) = gfpec_precom_nistP521r1_fun();

   return sts;
}
