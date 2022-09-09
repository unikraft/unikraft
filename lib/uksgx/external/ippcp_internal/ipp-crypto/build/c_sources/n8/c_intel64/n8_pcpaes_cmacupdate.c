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
//     AES-CMAC Functions
// 
//  Contents:
//        ippsAES_CMACUpdate()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpcmac.h"
#include "pcpaesm.h"
#include "pcptool.h"

#if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
#  include "pcprijtables.h"
#endif


/*F*
//    Name: ippsAES_CMACUpdate
//
// Purpose: Updates intermadiate digest based on input stream.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pSrc == NULL
//                            pState == NULL
//    ippStsContextMatchErr   !VALID_AESCMAC_ID()
//    ippStsLengthErr         len <0
//    ippStsNoErr             no errors
//
// Parameters:
//    pSrc     pointer to the input stream
//    len      input stream length
//    pState   pointer to the CMAC context
//
*F*/
static void AES_CMAC_processing(Ipp8u* pDigest, const Ipp8u* pSrc, int processedLen, const IppsAESSpec* pAES)
{
#if (_IPP>=_IPP_P8) || (_IPP32E>=_IPP32E_Y8)
   if(AES_NI_ENABLED==RIJ_AESNI(pAES)) {
      cpAESCMAC_Update_AES_NI(pDigest, pSrc, processedLen, RIJ_NR(pAES), RIJ_EKEYS(pAES));
   }
   else
#endif
   {
      /* setup encoder method */
      RijnCipher encoder = RIJ_ENCODER(pAES);

      while(processedLen) {
         ((Ipp32u*)pDigest)[0] ^= ((Ipp32u*)pSrc)[0];
         ((Ipp32u*)pDigest)[1] ^= ((Ipp32u*)pSrc)[1];
         ((Ipp32u*)pDigest)[2] ^= ((Ipp32u*)pSrc)[2];
         ((Ipp32u*)pDigest)[3] ^= ((Ipp32u*)pSrc)[3];

         #if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
         encoder(pDigest, pDigest, RIJ_NR(pAES), RIJ_EKEYS(pAES), RijEncSbox/*NULL*/);
         #else
         encoder(pDigest, pDigest, RIJ_NR(pAES), RIJ_EKEYS(pAES), NULL);
         #endif

         pSrc += MBS_RIJ128;
         processedLen -= MBS_RIJ128;
      }
   }
}

IPPFUN(IppStatus, ippsAES_CMACUpdate,(const Ipp8u* pSrc, int len, IppsAES_CMACState* pState))
{
   int processedLen;

   /* test context pointer */
   IPP_BAD_PTR1_RET(pState);
   /* test ID */
   IPP_BADARG_RET(!VALID_AESCMAC_ID(pState), ippStsContextMatchErr);
   /* test input message and it's length */
   IPP_BADARG_RET((len<0 && pSrc), ippStsLengthErr);
   /* test source pointer */
   IPP_BADARG_RET((len && !pSrc), ippStsNullPtrErr);

   if(!len)
      return ippStsNoErr;

   {
      /*
      // test internal buffer filling
      */
      if(CMAC_INDX(pState)) {
         /* copy from input stream to the internal buffer as match as possible */
         processedLen = IPP_MIN(len, (MBS_RIJ128 - CMAC_INDX(pState)));
         CopyBlock(pSrc, CMAC_BUFF(pState)+CMAC_INDX(pState), processedLen);

         /* internal buffer filling */
         CMAC_INDX(pState) += processedLen;

         /* update message pointer and length */
         pSrc += processedLen;
         len  -= processedLen;

         if(!len)
            return ippStsNoErr;

         /* update CMAC if buffer full but not the last */
         if(MBS_RIJ128==CMAC_INDX(pState) ) {
            const IppsAESSpec* pAES = &CMAC_CIPHER(pState);
            /* setup encoder method */
            RijnCipher encoder = RIJ_ENCODER(pAES);
            XorBlock16(CMAC_BUFF(pState), CMAC_MAC(pState), CMAC_MAC(pState));

            #if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
            encoder(CMAC_MAC(pState), CMAC_MAC(pState), RIJ_NR(pAES), RIJ_EKEYS(pAES), RijEncSbox/*NULL*/);
            #else
            encoder(CMAC_MAC(pState), CMAC_MAC(pState), RIJ_NR(pAES), RIJ_EKEYS(pAES), NULL);
            #endif

            CMAC_INDX(pState) = 0;
         }
      }

      /*
      // main part
      */
      processedLen = len & ~(MBS_RIJ128-1);
      if(!(len & (MBS_RIJ128-1)))
         processedLen -= MBS_RIJ128;
      if(processedLen) {
         const IppsAESSpec* pAES = &CMAC_CIPHER(pState);

         AES_CMAC_processing(CMAC_MAC(pState), pSrc, processedLen, pAES);

         /* update message pointer and length */
         pSrc += processedLen;
         len  -= processedLen;
      }

      /*
      // remaind
      */
      if(len) {
         CopyBlock_safe(pSrc, len, (Ipp8u*)(&CMAC_BUFF(pState)), MBS_RIJ128);
         /* update internal buffer filling */
         CMAC_INDX(pState) += len;
      }

      return ippStsNoErr;
   }
}
