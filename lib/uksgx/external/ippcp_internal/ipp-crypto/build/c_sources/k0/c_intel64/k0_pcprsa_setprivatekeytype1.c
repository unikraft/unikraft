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
//        ippsRSA_SetPrivateKeyType1()
//
*/

#include "owncp.h"
#include "pcpbn.h"
#include "pcpngrsa.h"

/*F*
// Name: ippsRSA_SetPrivateKeyType1
//
// Purpose: Set up the RSA private key
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pModulus
//                               NULL == pPrivateExp
//                               NULL == pKey
//
//    ippStsContextMatchErr     !BN_VALID_ID(pModulus)
//                              !BN_VALID_ID(pPrivateExp)
//                              !RSA_PRV_KEY_VALID_ID()
//
//    ippStsOutOfRangeErr        0 >= pModulus
//                               0 >= pPrivateExp
//
//    ippStsSizeErr              bitsize(pModulus) exceeds requested value
//                               bitsize(pPrivateExp) exceeds requested value
//
//    ippStsNoErr                no error
//
// Parameters:
//    pModulus       pointer to modulus (N)
//    pPrivateExp    pointer to public exponent (D)
//    pKey           pointer to the key context
*F*/
IPPFUN(IppStatus, ippsRSA_SetPrivateKeyType1,(const IppsBigNumState* pModulus,
                                              const IppsBigNumState* pPrivateExp,
                                              IppsRSAPrivateKeyState* pKey))
{
   IPP_BAD_PTR1_RET(pKey);
   IPP_BADARG_RET(!RSA_PRV_KEY1_VALID_ID(pKey), ippStsContextMatchErr);

   IPP_BAD_PTR1_RET(pModulus);
   IPP_BADARG_RET(!BN_VALID_ID(pModulus), ippStsContextMatchErr);
   IPP_BADARG_RET(!(0 < cpBN_tst(pModulus)), ippStsOutOfRangeErr);
   IPP_BADARG_RET(BITSIZE_BNU(BN_NUMBER(pModulus), BN_SIZE(pModulus)) > RSA_PRV_KEY_MAXSIZE_N(pKey), ippStsSizeErr);

   IPP_BAD_PTR1_RET(pPrivateExp);
   IPP_BADARG_RET(!BN_VALID_ID(pPrivateExp), ippStsContextMatchErr);
   IPP_BADARG_RET(!(0 < cpBN_tst(pPrivateExp)), ippStsOutOfRangeErr);
   IPP_BADARG_RET(BITSIZE_BNU(BN_NUMBER(pPrivateExp), BN_SIZE(pPrivateExp)) > RSA_PRV_KEY_MAXSIZE_D(pKey), ippStsSizeErr);

   {
      /* store D */
      ZEXPAND_COPY_BNU(RSA_PRV_KEY_D(pKey), BITS_BNU_CHUNK(RSA_PRV_KEY_MAXSIZE_D(pKey)), BN_NUMBER(pPrivateExp), BN_SIZE(pPrivateExp));

      /* setup montgomery engine */
      gsModEngineInit(RSA_PRV_KEY_NMONT(pKey), (Ipp32u*)BN_NUMBER(pModulus), cpBN_bitsize(pModulus), MOD_ENGINE_RSA_POOL_SIZE, gsModArithRSA());

      RSA_PRV_KEY_BITSIZE_N(pKey) = cpBN_bitsize(pModulus);
      RSA_PRV_KEY_BITSIZE_D(pKey) = cpBN_bitsize(pPrivateExp);

      return ippStsNoErr;
   }
}
