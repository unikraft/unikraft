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
//     SMS4 encryption/decryption
// 
//  Contents:
//        cpSMS4_Cipher()
//
*/

#include "owncp.h"
#include "pcpsms4.h"
#include "pcptool.h"


//#include "owndefs.h"
static void cpSMS4_ECB_gpr_x1(Ipp8u* otxt, const Ipp8u* itxt, const Ipp32u* pRoundKeys)
{
   __ALIGN16 Ipp32u buff[4 + SMS4_ROUND_KEYS_NUM];
   buff[0] = HSTRING_TO_U32(itxt);
   buff[1] = HSTRING_TO_U32(itxt+sizeof(Ipp32u));
   buff[2] = HSTRING_TO_U32(itxt+sizeof(Ipp32u)*2);
   buff[3] = HSTRING_TO_U32(itxt+sizeof(Ipp32u)*3);
   {
      int nr;
      for(nr=0; nr < SMS4_ROUND_KEYS_NUM; nr++) {
         buff[4+nr] = buff[nr] ^ cpCipherMix_SMS4(buff[nr+1]^buff[nr+2]^buff[nr+3]^pRoundKeys[nr]);
      }
   }
   U32_TO_HSTRING(otxt,                  buff[4 + SMS4_ROUND_KEYS_NUM - 1]);
   U32_TO_HSTRING(otxt+sizeof(Ipp32u),   buff[4 + SMS4_ROUND_KEYS_NUM - 2]);
   U32_TO_HSTRING(otxt+sizeof(Ipp32u)*2, buff[4 + SMS4_ROUND_KEYS_NUM - 3]);
   U32_TO_HSTRING(otxt+sizeof(Ipp32u)*3, buff[4 + SMS4_ROUND_KEYS_NUM - 4]);

   /* clear secret data */
   PurgeBlock(buff, sizeof(buff));
}

IPP_OWN_DEFN (void, cpSMS4_Cipher, (Ipp8u* otxt, const Ipp8u* itxt, const Ipp32u* pRoundKeys))
{
   #if (_IPP32E>=_IPP32E_K1)
   #if defined (__INTEL_COMPILER) || !defined (_MSC_VER) || (_MSC_VER >= 1920)
   if (IsFeatureEnabled(ippCPUID_AVX512GFNI)){
      cpSMS4_ECB_gfni_x1(otxt, itxt, pRoundKeys);
      return;
   }
   else
   #endif /* #if defined (__INTEL_COMPILER) || !defined (_MSC_VER) || (_MSC_VER >= 1920) */
   #endif
   #if (_IPP>=_IPP_P8) || (_IPP32E>=_IPP32E_Y8)
   if(IsFeatureEnabled(ippCPUID_AES)){
      cpSMS4_ECB_aesni_x1(otxt, itxt, pRoundKeys);
      return;
   }  
   else
   #endif
   {
      cpSMS4_ECB_gpr_x1(otxt, itxt, pRoundKeys);
      return;
   }
}
