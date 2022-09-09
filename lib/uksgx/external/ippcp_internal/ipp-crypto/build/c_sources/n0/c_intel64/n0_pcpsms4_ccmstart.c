/*******************************************************************************
* Copyright 2017-2021 Intel Corporation
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
//  Purpose:
//     Cryptography Primitive.
//     SMS4-CCM implementation.
// 
//     Content:
//        ippsSMS4_CCMStart()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpsms4authccm.h"
#include "pcptool.h"

/*F*
//    Name: ippsSMS4_CCMStart
//
// Purpose: Start the process (encryption+generation) or (decryption+veryfication).
//
// Returns:                Reason:
//    ippStsNullPtrErr        pCtx == NULL
//                            pIV == NULL
//                            pAD == NULL
//    ippStsContextMatchErr   !VALID_SMS4CCM_ID()
//    ippStsLengthErr         13 < ivLen < 7
//    ippStsNoErr             no errors
//
// Parameters:
//    pIV      pointer to the IV (nonce)
//    ivLen    length of the IV (in bytes)
//    pAD      pointer to the Associated Data (header)
//    adLen    length of the AD (in bytes)
//    pCtx     pointer to the CCM context
//
*F*/
IPPFUN(IppStatus, ippsSMS4_CCMStart,(const Ipp8u* pIV, int ivLen,
                                    const Ipp8u* pAD, int adLen,
                                    IppsSMS4_CCMState* pCtx))
{
   /* test pCtx pointer */
   IPP_BAD_PTR1_RET(pCtx);
   IPP_BADARG_RET(!VALID_SMS4CCM_ID(pCtx), ippStsContextMatchErr);

   /* test IV (or nonce) */
   IPP_BAD_PTR1_RET(pIV);
   IPP_BADARG_RET((ivLen<7)||(ivLen>13), ippStsLengthErr);

   /* test AAD pointer if defined */
   IPP_BADARG_RET(adLen<0, ippStsLengthErr);
   if(adLen)
      IPP_BAD_PTR1_RET(pAD);

   /* init for new message */
   SMS4CCM_LENPRO(pCtx) = 0;
   SMS4CCM_COUNTER(pCtx) = 0;

   {
      /* setup encoder function */
      IppsSMS4Spec* pSMS4 = SMS4CCM_CIPHER(pCtx);

      __ALIGN16 Ipp32u TMP[4*(MBS_SMS4/sizeof(Ipp32u)) + 8];
      /*
         MAC          size = MBS_SMS4/sizeof(Ipp32u)
         CTR          size = MBS_SMS4/sizeof(Ipp32u)
         block        size = 2*MBS_SMS4/sizeof(Ipp32u)
         qLen         size = 1
         qLenEnc      size = 1
         tagLenEnc    size = 1
         payloadLen   size = 2
         adLenEnc     size = 3
      */

      Ipp32u*          MAC = TMP;
      Ipp32u*          CTR = TMP + (MBS_SMS4/sizeof(Ipp32u));
      Ipp32u*        block = TMP + 2*(MBS_SMS4/sizeof(Ipp32u));
      Ipp32u*         qLen = TMP + 4*(MBS_SMS4/sizeof(Ipp32u));
      Ipp32u*      qLenEnc = TMP + 4*(MBS_SMS4/sizeof(Ipp32u)) + 1;
      Ipp32u*    tagLenEnc = TMP + 4*(MBS_SMS4/sizeof(Ipp32u)) + 2;
      Ipp32u*   payloadLen = TMP + 4*(MBS_SMS4/sizeof(Ipp32u)) + 3;
      Ipp32u*     adLenEnc = TMP + 4*(MBS_SMS4/sizeof(Ipp32u)) + 5;

      /*
      // prepare the 1-st input block B0 and encode
      */
      *qLen = (Ipp32u)( (MBS_SMS4-1) - ivLen);
      *qLenEnc = *qLen-1;

      *tagLenEnc = (SMS4CCM_TAGLEN(pCtx)-2)>>1;

      *((Ipp64u*)payloadLen) = SMS4CCM_MSGLEN(pCtx);

      ((Ipp8u*)MAC)[0] = (Ipp8u)( (Ipp32u)((adLen!=0) <<6) + (*tagLenEnc<<3) + *qLenEnc); /* flags */
      #if (IPP_ENDIAN == IPP_LITTLE_ENDIAN)
      MAC[2] = ENDIANNESS(IPP_HIDWORD(*((Ipp64u*)payloadLen)));
      MAC[3] = ENDIANNESS(IPP_LODWORD(*((Ipp64u*)payloadLen)));
      #else
      MAC[2] = IPP_HIDWORD(*((Ipp64u*)payloadLen));
      MAC[3] = IPP_LODWORD(*((Ipp64u*)payloadLen));
      #endif
      CopyBlock(pIV, ((Ipp8u*)MAC)+1, ivLen);

      /* setup CTR0 */
      FillBlock16(0, NULL,CTR, 0);
      ((Ipp8u*)CTR)[0] = (Ipp8u)(*qLenEnc); /* flags */
      CopyBlock(pIV, ((Ipp8u*)CTR)+1, ivLen);
      CopyBlock16(CTR, SMS4CCM_CTR0(pCtx));

      /* compute and store S0=ENC(CTR0) */
      cpSMS4_Cipher(SMS4CCM_S0(pCtx), (Ipp8u*)CTR, SMS4_RK(pSMS4));

      /*
      // update MAC by the AD
      */
      if(adLen) {
         /* encode length of associated data */
         Ipp8u* adLenEncPtr;
         int    adLenEncSize;

         #if (IPP_ENDIAN == IPP_LITTLE_ENDIAN)
         adLenEnc[1] = ENDIANNESS(IPP_HIDWORD(adLen));
         adLenEnc[2] = ENDIANNESS(IPP_LODWORD(adLen));
         #else
         adLenEnc[1] = IPP_HIDWORD(adLen);
         adLenEnc[2] = IPP_LODWORD(adLen);
         #endif

         if(adLen >= 0xFF00) {
            adLenEncSize = 6;
            #if (IPP_ENDIAN == IPP_LITTLE_ENDIAN)
            adLenEnc[1] = 0xFEFFFFFF;
            #else
            adLenEnc[1] = 0xFFFFFFFE;
            #endif
         }
         else {
            adLenEncSize= 2;
         }
         adLenEncPtr = (Ipp8u*)adLenEnc+3*sizeof(Ipp32u)-adLenEncSize;

         /* prepare first formatted block of Header */
         CopyBlock(adLenEncPtr, block, adLenEncSize);
         FillBlock16(0,pAD, (Ipp8u*)block+adLenEncSize, IPP_MIN((MBS_SMS4-adLenEncSize), adLen));

         /* and update MAC */
         MAC[0] ^= block[0];
         MAC[1] ^= block[1];
         MAC[2] ^= block[2];
         MAC[3] ^= block[3];
         cpSMS4_Cipher((Ipp8u*)MAC, (Ipp8u*)MAC, SMS4_RK(pSMS4));

         /* update MAC the by rest of addition data */
         if( (adLen+adLenEncSize) > MBS_SMS4 )  {
            pAD += (MBS_SMS4-adLenEncSize);
            adLen -= (MBS_SMS4-adLenEncSize);
            while(adLen >= MBS_SMS4) {
               CopyBlock16(pAD, block);
               MAC[0] ^= block[0];
               MAC[1] ^= block[1];
               MAC[2] ^= block[2];
               MAC[3] ^= block[3];
               cpSMS4_Cipher((Ipp8u*)MAC, (Ipp8u*)MAC, SMS4_RK(pSMS4));

               pAD += MBS_SMS4;
               adLen -= MBS_SMS4;
            }

            if(adLen) {
               FillBlock16(0, pAD, block, (int)adLen);
               MAC[0] ^= block[0];
               MAC[1] ^= block[1];
               MAC[2] ^= block[2];
               MAC[3] ^= block[3];
               cpSMS4_Cipher((Ipp8u*)MAC, (Ipp8u*)MAC, SMS4_RK(pSMS4));
            }
         }
      }

      SMS4CCM_COUNTER(pCtx) = 0;
      CopyBlock16(MAC, SMS4CCM_MAC(pCtx));

      /* clear secret data */
      PurgeBlock(TMP, sizeof(TMP));

      return ippStsNoErr;
   }
}
