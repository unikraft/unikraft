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
//        ippsRSA_GetBufferSizePublicKey()
//
*/

#include "owncp.h"
#include "pcpbn.h"
#include "pcpngrsa.h"
#include "pcpngrsamethod.h"

#include "pcprsa_getdefmeth_pub.h"

/*F*
// Name: ippsRSA_GetBufferSizePublicKey
//
// Purpose: Returns size of temporary buffer (in bytes) for public key operation
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pKey
//                               NULL == pBufferSize
//
//    ippStsContextMatchErr     !RSA_PUB_KEY_VALID_ID()
//
//    ippStsIncompleteContextErr no ippsRSA_SetPublicKey() call
//
//    ippStsNoErr                no error
//
// Parameters:
//    pBufferSize pointer to size of temporary buffer
//    pKey        pointer to the key context
*F*/
IPPFUN(IppStatus, ippsRSA_GetBufferSizePublicKey,(int* pBufferSize, const IppsRSAPublicKeyState* pKey))
{
   IPP_BAD_PTR1_RET(pKey);
   IPP_BADARG_RET(!RSA_PUB_KEY_VALID_ID(pKey), ippStsContextMatchErr);
   IPP_BADARG_RET(!RSA_PUB_KEY_IS_SET(pKey), ippStsIncompleteContextErr);

   IPP_BAD_PTR1_RET(pBufferSize);

   {
      cpSize bitSizeN = RSA_PUB_KEY_BITSIZE_N(pKey);
      cpSize nsN = BITS_BNU_CHUNK(bitSizeN);

      gsMethod_RSA* m = getDefaultMethod_RSA_public(bitSizeN);

      cpSize bufferNum = ((nsN+1)*2)*2          /* (1)2 BN for RSA (enc)/sign schemes */
                        + 1;                    /* BNU_CHUNK_T alignment */
      bufferNum += m->bufferNumFunc(bitSizeN);  /* RSA public key operation */

      *pBufferSize = bufferNum*(Ipp32s)sizeof(BNU_CHUNK_T);

      return ippStsNoErr;
   }
}
