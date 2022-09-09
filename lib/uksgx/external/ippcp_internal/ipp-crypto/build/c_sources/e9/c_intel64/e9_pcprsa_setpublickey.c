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
//        ippsRSA_SetPublicKey()
//
*/

#include "owncp.h"
#include "pcpbn.h"
#include "pcpngrsa.h"

/*F*
// Name: ippsRSA_SetPublicKey
//
// Purpose: Set up the RSA public key
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pKey
//                               NULL == pPublicExp
//                               NULL == pKey
//
//    ippStsContextMatchErr     !BN_VALID_ID(pModulus)
//                              !BN_VALID_ID(pPublicExp)
//                              !RSA_PUB_KEY_VALID_ID()
//
//    ippStsOutOfRangeErr        0 >= pModulus
//                               0 >= pPublicExp
//
//    ippStsSizeErr              bitsize(pModulus) exceeds requested value
//                               bitsize(pPublicExp) exceeds requested value
//
//    ippStsNoErr                no error
//
// Parameters:
//    pModulus       pointer to modulus (N)
//    pPublicExp     pointer to public exponent (E)
//    pKey           pointer to the key context
*F*/
IPPFUN(IppStatus, ippsRSA_SetPublicKey,(const IppsBigNumState* pModulus,
                                        const IppsBigNumState* pPublicExp,
                                        IppsRSAPublicKeyState* pKey))
{
   IPP_BAD_PTR1_RET(pKey);
   IPP_BADARG_RET(!RSA_PUB_KEY_VALID_ID(pKey), ippStsContextMatchErr);

   IPP_BAD_PTR1_RET(pModulus);
   IPP_BADARG_RET(!BN_VALID_ID(pModulus), ippStsContextMatchErr);
   IPP_BADARG_RET(!(0 < cpBN_tst(pModulus)), ippStsOutOfRangeErr);
   IPP_BADARG_RET(BITSIZE_BNU(BN_NUMBER(pModulus), BN_SIZE(pModulus)) > RSA_PUB_KEY_MAXSIZE_N(pKey), ippStsSizeErr);

   IPP_BAD_PTR1_RET(pPublicExp);
   IPP_BADARG_RET(!BN_VALID_ID(pPublicExp), ippStsContextMatchErr);
   IPP_BADARG_RET(!(0 < cpBN_tst(pPublicExp)), ippStsOutOfRangeErr);
   IPP_BADARG_RET(BITSIZE_BNU(BN_NUMBER(pPublicExp), BN_SIZE(pPublicExp)) > RSA_PUB_KEY_MAXSIZE_E(pKey), ippStsSizeErr);

   {
      RSA_PUB_KEY_BITSIZE_N(pKey) = 0;
      RSA_PUB_KEY_BITSIZE_E(pKey) = 0;

      /* store E */
      ZEXPAND_COPY_BNU(RSA_PUB_KEY_E(pKey), BITS_BNU_CHUNK(RSA_PUB_KEY_MAXSIZE_E(pKey)), BN_NUMBER(pPublicExp), BN_SIZE(pPublicExp));

      /* setup montgomery engine */
      gsModEngineInit(RSA_PUB_KEY_NMONT(pKey), (Ipp32u*)BN_NUMBER(pModulus), cpBN_bitsize(pModulus), MOD_ENGINE_RSA_POOL_SIZE, gsModArithRSA());

      RSA_PUB_KEY_BITSIZE_N(pKey) = cpBN_bitsize(pModulus);
      RSA_PUB_KEY_BITSIZE_E(pKey) = cpBN_bitsize(pPublicExp);

      return ippStsNoErr;
   }
}
