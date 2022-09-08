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
//        ippsRSA_GetBufferSizePrivateKey()
//
*/

#include "owncp.h"
#include "pcpbn.h"
#include "pcpngrsa.h"
#include "pcpngrsamethod.h"

#include "pcprsa_getdefmeth_priv.h"

/*F*
// Name: ippsRSA_GetBufferSizePrivateKey
//
// Purpose: Returns size of temporary buffer (in bytes) for private key operation
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pKey
//                               NULL == pBufferSize
//
//    ippStsContextMatchErr     !RSA_PRV_KEY_VALID_ID()
//
//    ippStsIncompleteContextErr (type1) private key is not set up
//
//    ippStsNoErr                no error
//
// Parameters:
//    pBufferSize pointer to size of temporary buffer
//    pKey        pointer to the key context
*F*/
IPPFUN(IppStatus, ippsRSA_GetBufferSizePrivateKey,(int* pBufferSize, const IppsRSAPrivateKeyState* pKey))
{
   IPP_BAD_PTR1_RET(pKey);
   IPP_BADARG_RET(!RSA_PRV_KEY_VALID_ID(pKey), ippStsContextMatchErr);
   IPP_BADARG_RET(RSA_PRV_KEY1_VALID_ID(pKey) && !RSA_PRV_KEY_IS_SET(pKey), ippStsIncompleteContextErr);

   IPP_BAD_PTR1_RET(pBufferSize);

   {
      cpSize modulusBits = (RSA_PRV_KEY1_VALID_ID(pKey))? RSA_PRV_KEY_BITSIZE_N(pKey) :
                                                  IPP_MAX(RSA_PRV_KEY_BITSIZE_P(pKey), RSA_PRV_KEY_BITSIZE_Q(pKey));
      gsMethod_RSA* m = getDualExpMethod_RSA_private(RSA_PRV_KEY_BITSIZE_P(pKey), RSA_PRV_KEY_BITSIZE_Q(pKey));
      if (NULL == m)
         m = getDefaultMethod_RSA_private(modulusBits);

      cpSize bitSizeN = (RSA_PRV_KEY1_VALID_ID(pKey))? modulusBits : modulusBits*2;
      cpSize nsN = BITS_BNU_CHUNK(bitSizeN);

      cpSize bn_scheme = (nsN+1)*2;    /* BN for RSA schemes */
      cpSize bn3_gen = (RSA_PRV_KEY2_VALID_ID(pKey))? (nsN+1)*2*3 : 0; /* 3 BN for generation/validation */

      cpSize bufferNum = bn_scheme*2               /* (1)2 BN for RSA (enc)/sign schemes */
                       + 1;                        /* BNU_CHUNK_T alignment */
      bufferNum += m->bufferNumFunc(modulusBits);  /* RSA private key operation */

      bufferNum = IPP_MAX(bufferNum, bn3_gen); /* generation/validation resource overlaps RSA resource  */

      *pBufferSize = bufferNum*(Ipp32s)sizeof(BNU_CHUNK_T);

      #if defined(_USE_WINDOW_EXP_)
      /* pre-computed table should be CACHE_LINE aligned*/
      *pBufferSize += CACHE_LINE_SIZE;
      #endif

      return ippStsNoErr;
   }
}
