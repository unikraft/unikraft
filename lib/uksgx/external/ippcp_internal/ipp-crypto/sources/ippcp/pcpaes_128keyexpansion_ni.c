/*******************************************************************************
* Copyright 2016-2021 Intel Corporation
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
//     Cryptography Primitive. AES keys expansion
// 
//  Contents:
//        aes128_KeyExpansion_NI()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpaesm.h"



#if (_AES_NI_ENABLING_==_FEATURE_ON_) || (_AES_NI_ENABLING_==_FEATURE_TICKTOCK_)

/*
// AES-128 key expansion
*/
static __m128i aes128_assist(__m128i temp1, __m128i temp2)
{
   __m128i temp3;
   temp2 = _mm_shuffle_epi32 (temp2 ,0xff);
   temp3 = _mm_slli_si128 (temp1, 0x4);
   temp1 = _mm_xor_si128 (temp1, temp3);
   temp3 = _mm_slli_si128 (temp3, 0x4);
   temp1 = _mm_xor_si128 (temp1, temp3);
   temp3 = _mm_slli_si128 (temp3, 0x4);
   temp1 = _mm_xor_si128 (temp1, temp3);
   temp1 = _mm_xor_si128 (temp1, temp2);
   return temp1;
}

#define aes128_KeyExpansion_NI OWNAPI(aes128_KeyExpansion_NI)
   IPP_OWN_DECL (void, aes128_KeyExpansion_NI, (Ipp8u* keyExp, const Ipp8u* userkey))

IPP_OWN_DEFN (void, aes128_KeyExpansion_NI, (Ipp8u* keyExp, const Ipp8u* userkey))
{
   __m128i *pKeySchedule = (__m128i*)keyExp;

   __m128i temp[2];
   /*
      temp[0] = temp1
      temp[1] = temp2
   */

   temp[0] = _mm_loadu_si128((__m128i*)userkey);
   pKeySchedule[0] = temp[0];
   temp[1] = _mm_aeskeygenassist_si128 (temp[0] ,0x1);
   temp[0] = aes128_assist(temp[0], temp[1]);
   pKeySchedule[1] = temp[0];
   temp[1] = _mm_aeskeygenassist_si128 (temp[0],0x2);
   temp[0] = aes128_assist(temp[0], temp[1]);
   pKeySchedule[2] = temp[0];
   temp[1] = _mm_aeskeygenassist_si128 (temp[0],0x4);
   temp[0] = aes128_assist(temp[0], temp[1]);
   pKeySchedule[3] = temp[0];
   temp[1] = _mm_aeskeygenassist_si128 (temp[0],0x8);
   temp[0] = aes128_assist(temp[0], temp[1]);
   pKeySchedule[4] = temp[0];
   temp[1] = _mm_aeskeygenassist_si128 (temp[0],0x10);
   temp[0] = aes128_assist(temp[0], temp[1]);
   pKeySchedule[5] = temp[0];
   temp[1] = _mm_aeskeygenassist_si128 (temp[0],0x20);
   temp[0] = aes128_assist(temp[0], temp[1]);
   pKeySchedule[6] = temp[0];
   temp[1] = _mm_aeskeygenassist_si128 (temp[0],0x40);
   temp[0] = aes128_assist(temp[0], temp[1]);
   pKeySchedule[7] = temp[0];
   temp[1] = _mm_aeskeygenassist_si128 (temp[0],0x80);
   temp[0] = aes128_assist(temp[0], temp[1]);
   pKeySchedule[8] = temp[0];
   temp[1] = _mm_aeskeygenassist_si128 (temp[0],0x1b);
   temp[0] = aes128_assist(temp[0], temp[1]);
   pKeySchedule[9] = temp[0];
   temp[1] = _mm_aeskeygenassist_si128 (temp[0],0x36);
   temp[0] = aes128_assist(temp[0], temp[1]);
   pKeySchedule[10] = temp[0];

   /* clear secret data */
   for(int i = 0; i < sizeof(temp)/sizeof(temp[0]); i++){
      temp[i] = _mm_xor_si128(temp[i],temp[i]);
   }
}

#endif /* #if (_AES_NI_ENABLING_==_FEATURE_ON_) || (_AES_NI_ENABLING_==_FEATURE_TICKTOCK_) */

