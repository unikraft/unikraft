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
//     Intel(R) Integrated Performance Primitives. Cryptography Primitives.
//
//     Context:
//        ippsAES_CCMEncrypt()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpaesauthccm.h"
#include "pcptool.h"

#if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
#  include "pcprijtables.h"
#endif

/*F*
//    Name: ippsAES_CCMEncrypt
//
// Purpose: Encrypts data and updates authentication tag.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pState== NULL
//                            pSrc == NULL
//                            pDst == NULL
//    ippStsContextMatchErr   !VALID_AESCCM_ID()
//    ippStsLengthErr         if exceed overall length of message is being processed
//    ippStsNoErr             no errors
//
// Parameters:
//    pSrc        pointer to the plane text beffer
//    pDst        pointer to the cipher text bubber
//    len         length of the buffer
//    pState      pointer to the CCM context
//
*F*/
IPPFUN(IppStatus, ippsAES_CCMEncrypt,(const Ipp8u* pSrc, Ipp8u* pDst, int len, IppsAES_CCMState* pState))
{
   /* test pState pointer */
   IPP_BAD_PTR1_RET(pState);
   IPP_BADARG_RET(!VALID_AESCCM_ID(pState), ippStsContextMatchErr);

   /* test source/destination data */
   IPP_BAD_PTR2_RET(pSrc, pDst);

   /* test message length */
   IPP_BADARG_RET(len<0 || AESCCM_LENPRO(pState)+(Ipp64u)len >AESCCM_MSGLEN(pState), ippStsLengthErr);

   /*
   // enctypt payload and update MAC
   */
   if(len) {
      /* setup encoder method */
      IppsAESSpec* pAES = AESCCM_CIPHER(pState);
      RijnCipher encoder = RIJ_ENCODER(pAES);

      Ipp32u flag = (Ipp32u)( AESCCM_LENPRO(pState) &(MBS_RIJ128-1) );

      Ipp32u qLen;
      Ipp32u counterVal;

      Ipp32u MAC[NB(128)];
      Ipp32u CTR[NB(128)];
      Ipp32u   S[NB(128)];
      /* extract from the state */
      CopyBlock16(AESCCM_MAC(pState), MAC);
      CopyBlock16(AESCCM_CTR0(pState), CTR);
      CopyBlock16(AESCCM_Si(pState), S);
      counterVal = AESCCM_COUNTER(pState);

      /* extract qLen */
      qLen = (((Ipp8u*)CTR)[0] &0x7) +1; /* &0x7 just to fix KW issue */

      if(flag) {
         Ipp32u tmpLen = (Ipp32u)IPP_MIN(len, MBS_RIJ128-1);

         /* copy as much input as possible into the internal buffer*/
         CopyBlock(pSrc, AESCCM_BLK(pState)+flag, (Ipp32s)tmpLen);

         XorBlock(pSrc, (Ipp8u*)S+flag, pDst, (Ipp32s)tmpLen);

         /* update MAC */
         if(flag+(Ipp32u)tmpLen == MBS_RIJ128) {
            XorBlock16(MAC, AESCCM_BLK(pState), MAC);
            #if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
            encoder((Ipp8u*)MAC, (Ipp8u*)MAC, RIJ_NR(pAES), RIJ_EKEYS(pAES), RijEncSbox/*NULL*/);
            #else
            encoder((Ipp8u*)MAC, (Ipp8u*)MAC, RIJ_NR(pAES), RIJ_EKEYS(pAES), NULL);
            #endif
         }

         AESCCM_LENPRO(pState) += (Ipp32u)tmpLen;
         pSrc += tmpLen;
         pDst += tmpLen;
         len  -= tmpLen;
      }

      #if (_IPP>=_IPP_P8) || (_IPP32E>=_IPP32E_Y8)
      if(AES_NI_ENABLED==RIJ_AESNI(pAES)) {
         Ipp32u processedLen = (Ipp32u)(len & -MBS_RIJ128);
         if(processedLen) {
            /* local state: MAC, counter block, counter bits mask */
            __ALIGN16 Ipp8u localState[3*MBS_RIJ128];

            /* format counter block and fill local state: */
            Ipp32u n;
            for(n=0; n<MBS_RIJ128-qLen; n++) localState[MBS_RIJ128*2+n] = 0;
            for(n=MBS_RIJ128-qLen; n<MBS_RIJ128; n++) localState[MBS_RIJ128*2+n] = 0xFF;
            CopyBlock(CounterEnc((Ipp32u*)localState, (Ipp32s)qLen, counterVal), ((Ipp8u*)CTR)+MBS_RIJ128-qLen, (Ipp32s)qLen);
            CopyBlock(CTR, localState+MBS_RIJ128, MBS_RIJ128);
            CopyBlock(MAC, localState, MBS_RIJ128);

            /* encrypt and authenticate */
            AuthEncrypt_RIJ128_AES_NI(pSrc, pDst, RIJ_NR(pAES), RIJ_EKEYS(pAES), processedLen, localState);

            /* update parameters */
            CopyBlock(localState, MAC, MBS_RIJ128);
            CopyBlock(localState+MBS_RIJ128, S, MBS_RIJ128);
            counterVal += processedLen/MBS_RIJ128;

            pSrc += processedLen;
            pDst += processedLen;
            len -= processedLen;
         }
      }
      #endif

      while(len >= MBS_RIJ128) {
         Ipp32u counterEnc[2];

         /* update MAC */
         XorBlock16(MAC, pSrc, MAC);
         #if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
         encoder((Ipp8u*)MAC, (Ipp8u*)MAC, RIJ_NR(pAES), RIJ_EKEYS(pAES), RijEncSbox/*NULL*/);
         #else
         encoder((Ipp8u*)MAC, (Ipp8u*)MAC, RIJ_NR(pAES), RIJ_EKEYS(pAES), NULL);
         #endif

         /* increment counter and format counter block */
         counterVal++;
         CopyBlock(CounterEnc(counterEnc, (Ipp32s)qLen, counterVal), ((Ipp8u*)CTR)+MBS_RIJ128-qLen, (Ipp32s)qLen);
         /* encode counter block */
         #if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
         encoder((Ipp8u*)CTR, (Ipp8u*)S, RIJ_NR(pAES), RIJ_EKEYS(pAES), RijEncSbox/*NULL*/);
         #else
         encoder((Ipp8u*)CTR, (Ipp8u*)S, RIJ_NR(pAES), RIJ_EKEYS(pAES), NULL);
         #endif

         /* store cipher text */
         XorBlock16(pSrc, S, pDst);

         AESCCM_LENPRO(pState) += MBS_RIJ128;
         pSrc += MBS_RIJ128;
         pDst += MBS_RIJ128;
         len  -= MBS_RIJ128;
      }

      if(len) {
         Ipp32u counterEnc[2];

         /* store partial data block */
         CopyBlock(pSrc, AESCCM_BLK(pState), len);

         /* increment counter and format counter block */
         counterVal++;
         CopyBlock(CounterEnc(counterEnc, (Ipp32s)qLen, counterVal), ((Ipp8u*)CTR)+MBS_RIJ128-qLen, (Ipp32s)qLen);
         /* encode counter block */
         #if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
         encoder((Ipp8u*)CTR, (Ipp8u*)S, RIJ_NR(pAES), RIJ_EKEYS(pAES), RijEncSbox/*NULL*/);
         #else
         encoder((Ipp8u*)CTR, (Ipp8u*)S, RIJ_NR(pAES), RIJ_EKEYS(pAES), NULL);
         #endif

         /* store cipher text */
         XorBlock(pSrc, S, pDst, len);

         AESCCM_LENPRO(pState) += (Ipp64u)len;
      }

      /* update state */
      CopyBlock16(MAC, AESCCM_MAC(pState));
      CopyBlock16(S, AESCCM_Si(pState));
      AESCCM_COUNTER(pState) = counterVal;

      /* clear secret data */
      PurgeBlock(S, sizeof(S));
   }

   return ippStsNoErr;
}
