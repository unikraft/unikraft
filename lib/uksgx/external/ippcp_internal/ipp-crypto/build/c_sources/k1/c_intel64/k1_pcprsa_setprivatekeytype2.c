/*******************************************************************************
* Copyright 2013-2021 Intel Corporation
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
// 
//  Purpose:
//     Cryptography Primitive.
//     RSA Functions
// 
//  Contents:
//        ippsRSA_SetPrivateKeyType2()
//
*/

#include "owncp.h"
#include "pcpbn.h"
#include "pcpngrsa.h"

/*F*
// Name: ippsRSA_SetPrivateKeyType2
//
// Purpose: Set up the RSA private key
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pFactorP, NULL == pFactorQ
//                               NULL == pCrtExpP, NULL == pCrtExpQ
//                               NULL == pInverseQ
//                               NULL == pKey
//
//    ippStsContextMatchErr     !BN_VALID_ID(pFactorP), !BN_VALID_ID(pFactorQ)
//                              !BN_VALID_ID(pCrtExpP), !BN_VALID_ID(pCrtExpQ)
//                              !BN_VALID_ID(pInverseQ)
//                              !RSA_PRV_KEY_VALID_ID()
//
//    ippStsOutOfRangeErr        0 >= pFactorP, 0 >= pFactorQ
//                               0 >= pCrtExpP, 0 >= pCrtExpQ
//                               0 >= pInverseQ
//
//    ippStsSizeErr              bitsize(pFactorP) exceeds requested value
//                               bitsize(pFactorQ) exceeds requested value
//                               bitsize(pCrtExpP) > bitsize(pFactorP)
//                               bitsize(pCrtExpQ) > bitsize(pFactorQ)
//                               bitsize(pInverseQ) > bitsize(pFactorP)
//
//    ippStsNoErr                no error
//
// Parameters:
//    pFactorP, pFactorQ   pointer to the RSA modulus (N) prime factors
//    pCrtExpP, pCrtExpQ   pointer to CTR's exponent
//    pInverseQ            1/Q mod P
//    pKey                 pointer to the key context
*F*/
IPPFUN(IppStatus, ippsRSA_SetPrivateKeyType2,(const IppsBigNumState* pFactorP,
                                              const IppsBigNumState* pFactorQ,
                                              const IppsBigNumState* pCrtExpP,
                                              const IppsBigNumState* pCrtExpQ,
                                              const IppsBigNumState* pInverseQ,
                                              IppsRSAPrivateKeyState* pKey))
{
   IPP_BAD_PTR1_RET(pKey);
   IPP_BADARG_RET(!RSA_PRV_KEY2_VALID_ID(pKey), ippStsContextMatchErr);

   IPP_BAD_PTR1_RET(pFactorP);
   IPP_BADARG_RET(!BN_VALID_ID(pFactorP), ippStsContextMatchErr);
   IPP_BADARG_RET(!(0 < cpBN_tst(pFactorP)), ippStsOutOfRangeErr);
   IPP_BADARG_RET(BITSIZE_BNU(BN_NUMBER(pFactorP), BN_SIZE(pFactorP)) > RSA_PRV_KEY_BITSIZE_P(pKey), ippStsSizeErr);

   IPP_BAD_PTR1_RET(pFactorQ);
   IPP_BADARG_RET(!BN_VALID_ID(pFactorQ), ippStsContextMatchErr);
   IPP_BADARG_RET(!(0 < cpBN_tst(pFactorQ)), ippStsOutOfRangeErr);
   IPP_BADARG_RET(BITSIZE_BNU(BN_NUMBER(pFactorQ), BN_SIZE(pFactorQ)) > RSA_PRV_KEY_BITSIZE_Q(pKey), ippStsSizeErr);

   /* let P>Q */
   //IPP_BADARG_RET(0>=cpBN_cmp(pFactorP,pFactorQ), ippStsBadArgErr);

   IPP_BAD_PTR1_RET(pCrtExpP);
   IPP_BADARG_RET(!BN_VALID_ID(pCrtExpP), ippStsContextMatchErr);
   IPP_BADARG_RET(!(0 < cpBN_tst(pCrtExpP)), ippStsOutOfRangeErr);
   IPP_BADARG_RET(BITSIZE_BNU(BN_NUMBER(pCrtExpP), BN_SIZE(pCrtExpP)) > RSA_PRV_KEY_BITSIZE_P(pKey), ippStsSizeErr);

   IPP_BAD_PTR1_RET(pCrtExpQ);
   IPP_BADARG_RET(!BN_VALID_ID(pCrtExpQ), ippStsContextMatchErr);
   IPP_BADARG_RET(!(0 < cpBN_tst(pCrtExpQ)), ippStsOutOfRangeErr);
   IPP_BADARG_RET(BITSIZE_BNU(BN_NUMBER(pCrtExpQ), BN_SIZE(pCrtExpQ)) > RSA_PRV_KEY_BITSIZE_Q(pKey), ippStsSizeErr);

   IPP_BAD_PTR1_RET(pInverseQ);
   IPP_BADARG_RET(!BN_VALID_ID(pInverseQ), ippStsContextMatchErr);
   IPP_BADARG_RET(!(0 < cpBN_tst(pInverseQ)), ippStsOutOfRangeErr);
   IPP_BADARG_RET(BITSIZE_BNU(BN_NUMBER(pInverseQ), BN_SIZE(pInverseQ)) > RSA_PRV_KEY_BITSIZE_P(pKey), ippStsSizeErr);

   /* set bitsize(N) = 0, so the key contex is not ready */
   RSA_PRV_KEY_BITSIZE_N(pKey) = 0;
   RSA_PRV_KEY_BITSIZE_D(pKey) = 0;

   /* setup montgomery engine P */
   gsModEngineInit(RSA_PRV_KEY_PMONT(pKey), (Ipp32u*)BN_NUMBER(pFactorP), cpBN_bitsize(pFactorP), MOD_ENGINE_RSA_POOL_SIZE, gsModArithRSA());
   /* setup montgomery engine Q */
   gsModEngineInit(RSA_PRV_KEY_QMONT(pKey), (Ipp32u*)BN_NUMBER(pFactorQ), cpBN_bitsize(pFactorQ), MOD_ENGINE_RSA_POOL_SIZE, gsModArithRSA());

   /* actual size of key components */
   RSA_PRV_KEY_BITSIZE_P(pKey) = cpBN_bitsize(pFactorP);
   RSA_PRV_KEY_BITSIZE_Q(pKey) = cpBN_bitsize(pFactorQ);

   /* store CTR's exp dp */
   ZEXPAND_COPY_BNU(RSA_PRV_KEY_DP(pKey), BITS_BNU_CHUNK(RSA_PRV_KEY_BITSIZE_P(pKey)), BN_NUMBER(pCrtExpP), BN_SIZE(pCrtExpP));
   /* store CTR's exp dq */
   ZEXPAND_COPY_BNU(RSA_PRV_KEY_DQ(pKey), BITS_BNU_CHUNK(RSA_PRV_KEY_BITSIZE_Q(pKey)), BN_NUMBER(pCrtExpQ), BN_SIZE(pCrtExpQ));
   /* store CTR's invQ */
   ZEXPAND_COPY_BNU(RSA_PRV_KEY_INVQ(pKey), BITS_BNU_CHUNK(RSA_PRV_KEY_BITSIZE_P(pKey)), BN_NUMBER(pInverseQ), BN_SIZE(pInverseQ));

   /* setup montgomery engine N = P*Q */
   {
      BNU_CHUNK_T* pN = MOD_MODULUS(RSA_PRV_KEY_NMONT(pKey));
      cpSize nsN = BITS_BNU_CHUNK(RSA_PRV_KEY_BITSIZE_P(pKey) + RSA_PRV_KEY_BITSIZE_Q(pKey));

      cpMul_BNU_school(pN,
                       BN_NUMBER(pFactorP), BN_SIZE(pFactorP),
                       BN_NUMBER(pFactorQ), BN_SIZE(pFactorQ));

      gsModEngineInit(RSA_PRV_KEY_NMONT(pKey), (Ipp32u*)MOD_MODULUS(RSA_PRV_KEY_NMONT(pKey)),
         RSA_PRV_KEY_BITSIZE_P(pKey) + RSA_PRV_KEY_BITSIZE_Q(pKey), MOD_ENGINE_RSA_POOL_SIZE, gsModArithRSA());

      FIX_BNU(pN, nsN);
      RSA_PRV_KEY_BITSIZE_N(pKey) = BITSIZE_BNU(pN, nsN);
   }

   return ippStsNoErr;
}
