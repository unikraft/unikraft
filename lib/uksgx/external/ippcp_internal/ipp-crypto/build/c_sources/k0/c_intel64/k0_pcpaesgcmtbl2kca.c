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
// 
//  Purpose:
//     Cryptography Primitive.
//     Encrypt/Decrypt byte data stream according to Rijndael128 (GCM mode)
// 
//     "fast" stuff
// 
//  Contents:
//      AesGcmAuth_table2K()
// 
*/


#include "owndefs.h"
#include "owncp.h"

#include "pcpaesauthgcm.h"
#include "pcptool.h"

/*
// authenticates n*BLOCK_SIZE bytes
*/
//#if !((_IPP==_IPP_V8) || (_IPP==_IPP_P8) || \
//      (_IPP==_IPP_S8) || (_IPP>=_IPP_G9) || \
//      (_IPP32E==_IPP32E_U8) || (_IPP32E==_IPP32E_Y8) || \
//      (_IPP32E==_IPP32E_N8) || (_IPP32E>=_IPP32E_E9))
#if 0
IPP_OWN_DEFN (void, AesGcmAuth_table2K, (Ipp8u* pHash, const Ipp8u* pSrc, int len, const Ipp8u* pHKey, const void* pParam))
{
   IPP_UNREFERENCED_PARAMETER(pParam);

   while(len>=BLOCK_SIZE) {
      /* add src */
      XorBlock16(pSrc, pHash, pHash);
      /* hash it */
      AesGcmMulGcm_table2K(pHash, pHKey, AesGcmConst_table);

      pSrc += BLOCK_SIZE;
      len -= BLOCK_SIZE;
   }
}
#endif

#if(_IPP32E<_IPP32E_K0)

IPP_OWN_DEFN (void, AesGcmAuth_table2K_ct, (Ipp8u* pHash, const Ipp8u* pSrc, int len, const Ipp8u* pHKey, const void* pParam))
{
   IPP_UNREFERENCED_PARAMETER(pParam);

   while (len >= BLOCK_SIZE) {
      /* add src */
      XorBlock16(pSrc, pHash, pHash);
      /* hash it */
      AesGcmMulGcm_table2K_ct(pHash, pHKey, AesGcmConst_table);

      pSrc += BLOCK_SIZE;
      len -= BLOCK_SIZE;
   }
}

#endif

//#endif

