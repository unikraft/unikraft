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
//        cpProcessSMS4_ctr()
//
*/

#include "owncp.h"
#include "pcpsms4.h"
#include "pcptool.h"

/*
// SMS4-CTR processing.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pCtx == NULL
//                            pSrc == NULL
//                            pDst == NULL
//                            pCtrValue ==NULL
//    ippStsContextMatchErr   !VALID_SMS4_ID()
//    ippStsLengthErr         len <1
//    ippStsCTRSizeErr        128 < ctrNumBitSize < 1
//    ippStsCTRSizeErr        data blocks number > 2^ctrNumBitSize
//    ippStsNoErr             no errors
//
// Parameters:
//    pSrc           pointer to the source data buffer
//    pDst           pointer to the target data buffer
//    dataLen        input/output buffer length (in bytes)
//    pCtx           pointer to rge SMS4 context
//    pCtrValue      pointer to the counter block
//    ctrNumBitSize  counter block size (bits)
//
// Note:
//    counter will updated on return
//
*/
IPP_OWN_DEFN (IppStatus, cpProcessSMS4_ctr, (const Ipp8u* pSrc, Ipp8u* pDst, int dataLen, const IppsSMS4Spec* pCtx, Ipp8u* pCtrValue, int ctrNumBitSize))
{
   /* test context */
   IPP_BAD_PTR1_RET(pCtx);
   /* test the context ID */
   IPP_BADARG_RET(!VALID_SMS4_ID(pCtx), ippStsContextMatchErr);

   /* test source, target and counter block pointers */
   IPP_BAD_PTR3_RET(pSrc, pDst, pCtrValue);
   /* test stream length */
   IPP_BADARG_RET((dataLen<1), ippStsLengthErr);

   /* test counter block size */
   IPP_BADARG_RET(((MBS_SMS4*8)<ctrNumBitSize)||(ctrNumBitSize<1), ippStsCTRSizeErr);

   /* test counter overflow */
   if(ctrNumBitSize < (8 * sizeof(int) - 5))
   {
      /*
      // dataLen is int, and it is always positive   
      // data blocks number compute from dataLen     
      // by dividing it to MBS_SMS4 = 16           
      // and additing 1 if dataLen % 16 != 0         
      // so if ctrNumBitSize >= 8 * sizeof(int) - 5                      
      // function can process data with any possible 
      // passed dataLen without counter overflow     
      */
      
      int dataBlocksNum = dataLen >> 4;
      if(dataLen & 15){
         dataBlocksNum++;
      }

      IPP_BADARG_RET(dataBlocksNum > (1 << ctrNumBitSize), ippStsCTRSizeErr);
   }

   {
      __ALIGN16 Ipp8u TMP[2*MBS_SMS4+1];

      /*
         maskIV    size = MBS_SMS4
         output    size = MBS_SMS4
         counter   size = MBS_SMS4
         maskValue size = 1
      */

      Ipp8u* output    = TMP;
      Ipp8u* counter   = TMP + MBS_SMS4;

      /* copy counter */
      CopyBlock16(pCtrValue, counter);

      /* do CTR processing */
      #if (_IPP>=_IPP_P8) || (_IPP32E>=_IPP32E_Y8)

      Ipp8u* maskIV    = TMP;
      /* output is not used together with maskIV, so this is why buffer for both values is the same */
      Ipp8u* maskValue = TMP + 2*MBS_SMS4;

      if(dataLen>=4*MBS_SMS4) {
         /* construct ctr mask */
         int n;
         int maskPosition = (MBS_SMS4*8-ctrNumBitSize)/8;
         *maskValue = (Ipp8u)(0xFF >> (MBS_SMS4*8-ctrNumBitSize)%8 );

         for(n=0; n<maskPosition; n++)
            maskIV[n] = 0;
         maskIV[maskPosition] = *maskValue;
         for(n=maskPosition+1; n < MBS_SMS4; n++)
            maskIV[n] = 0xFF;

         int processedLen = 0;

         #if (_IPP32E>=_IPP32E_K1)
         #if defined (__INTEL_COMPILER) || !defined (_MSC_VER) || (_MSC_VER >= 1920)
         if (IsFeatureEnabled(ippCPUID_AVX512GFNI)) {
            processedLen = cpSMS4_CTR_gfni512(pDst, pSrc, dataLen, SMS4_RK(pCtx), maskIV, counter);
         }
         else
         #endif /* #if defined (__INTEL_COMPILER) || !defined (_MSC_VER) || (_MSC_VER >= 1920) */
         #endif /* (_IPP32E>=_IPP32E_K1) */
         if(IsFeatureEnabled(ippCPUID_AES)) {
            processedLen = cpSMS4_CTR_aesni(pDst, pSrc, dataLen, SMS4_RK(pCtx), maskIV, counter);
         }

         pSrc += processedLen;
         pDst += processedLen;
         dataLen -= processedLen;

      }
      #endif

      {

         /* block-by-block processing */
         while(dataLen>= MBS_SMS4) {
            /* encrypt counter block */
            cpSMS4_Cipher((Ipp8u*)output, (Ipp8u*)counter,  SMS4_RK(pCtx));
            /* compute ciphertext block */
            XorBlock16(pSrc, output, pDst);
            /* increment counter block */
            StdIncrement(counter,MBS_SMS4*8, ctrNumBitSize);

            pSrc += MBS_SMS4;
            pDst += MBS_SMS4;
            dataLen -= MBS_SMS4;
         }

         /* last data block processing */
         if(dataLen) {
            /* encrypt counter block */
            cpSMS4_Cipher((Ipp8u*)output, (Ipp8u*)counter, SMS4_RK(pCtx));
            /* compute ciphertext block */
            XorBlock(pSrc, output, pDst,dataLen);
            /* increment counter block */
            StdIncrement((Ipp8u*)counter,MBS_SMS4*8, ctrNumBitSize);
         }

      }

      /* update counter */
      CopyBlock16(counter, pCtrValue);

      /* clear secret data */
      PurgeBlock(TMP, sizeof(TMP));
      
      return ippStsNoErr;
   }
}
