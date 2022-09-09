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
//        ippsSMS4DecryptCFB()
//
*/

#include "owncp.h"
#include "pcpsms4.h"
#include "pcptool.h"


/*F*
//    Name: ippsSMS4DecryptCFB
//
// Purpose: SMS4-CFB decryption.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pCtx == NULL
//                            pSrc == NULL
//                            pDst == NULL
//                            pIV  == NULL
//    ippStsContextMatchErr   !VALID_SMS4_ID()
//    ippStsLengthErr         len <1
//    ippStsCFBSizeErr        (1>cfbBlkSize || cfbBlkSize>MBS_SMS4)
//    ippStsUnderRunErr       0!=(len%cfbBlkSize)
//    ippStsNoErr             no errors
//
// Parameters:
//    pSrc        pointer to the source data buffer
//    pDst        pointer to the target data buffer
//    len         input/output buffer length (in bytes)
//    cfbBlkSize  CFB block size (in bytes)
//    pCtx        pointer to the SMS4 context
//    pIV         pointer to the initialization vector
//
*F*/
static
void cpDecryptSMS4_cfb(const Ipp8u* pIV,
                      const Ipp8u* pSrc, Ipp8u* pDst, int nBlocks, int cfbBlkSize,
                      const IppsSMS4Spec* pCtx)
{
   __ALIGN16 Ipp32u TMP[3*MBS_SMS4/sizeof(Ipp32u)];

   /*
      tmpInp size = 2*MBS_SMS4/sizeof(Ipp32u)
      tmpOut size =   MBS_SMS4/sizeof(Ipp32u)
   */

   Ipp32u* tmpInp = TMP; 
   Ipp32u* tmpOut = TMP + 2*MBS_SMS4/sizeof(Ipp32u);

   /* read IV */
   CopyBlock16(pIV, tmpInp);

   /* decrypt data block-by-block of cfbLen each */
   while(nBlocks) {
      /* decryption */
      cpSMS4_Cipher((Ipp8u*)tmpOut, (Ipp8u*)tmpInp, SMS4_RK(pCtx));

      /* store output and put feedback into the input buffer (tmpInp) */
      if( cfbBlkSize==MBS_SMS4 && pSrc!=pDst) {
         ((Ipp32u*)pDst)[0] = tmpOut[0]^((Ipp32u*)pSrc)[0];
         ((Ipp32u*)pDst)[1] = tmpOut[1]^((Ipp32u*)pSrc)[1];
         ((Ipp32u*)pDst)[2] = tmpOut[2]^((Ipp32u*)pSrc)[2];
         ((Ipp32u*)pDst)[3] = tmpOut[3]^((Ipp32u*)pSrc)[3];

         tmpInp[0] = ((Ipp32u*)pSrc)[0];
         tmpInp[1] = ((Ipp32u*)pSrc)[1];
         tmpInp[2] = ((Ipp32u*)pSrc)[2];
         tmpInp[3] = ((Ipp32u*)pSrc)[3];
      }
      else {
         int n;
         for(n=0; n<cfbBlkSize; n++) {
            ((Ipp8u*)tmpInp)[MBS_SMS4+n] = pSrc[n];
            pDst[n] = (Ipp8u)( ((Ipp8u*)tmpOut)[n] ^ pSrc[n] );
         }

         /* shift input buffer (tmpInp) for the next CFB operation */
         CopyBlock16((Ipp8u*)tmpInp+cfbBlkSize, tmpInp);
      }

      pSrc += cfbBlkSize;
      pDst += cfbBlkSize;
      nBlocks--;
   }

   PurgeBlock(TMP, sizeof(TMP));
}

IPPFUN(IppStatus, ippsSMS4DecryptCFB,(const Ipp8u* pSrc, Ipp8u* pDst, int len, int cfbBlkSize,
                                      const IppsSMS4Spec* pCtx,
                                      const Ipp8u* pIV))
{
   /* test context */
   IPP_BAD_PTR1_RET(pCtx);
   /* test the context ID */
   IPP_BADARG_RET(!VALID_SMS4_ID(pCtx), ippStsContextMatchErr);

   /* test source, target buffers and initialization pointers */
   IPP_BAD_PTR3_RET(pSrc, pIV, pDst);
   /* test stream length */
   IPP_BADARG_RET((len<1), ippStsLengthErr);
   /* test CFB value */
   IPP_BADARG_RET(((1>cfbBlkSize) || (MBS_SMS4<cfbBlkSize)), ippStsCFBSizeErr);

   /* test stream integrity */
   IPP_BADARG_RET((len%cfbBlkSize), ippStsUnderRunErr);

   /* do encryption */
   #if (_IPP32E>=_IPP32E_K1)
   #if defined (__INTEL_COMPILER) || !defined (_MSC_VER) || (_MSC_VER >= 1920)
   if (IsFeatureEnabled(ippCPUID_AVX512GFNI)) {

      __ALIGN16 Ipp8u IV[MBS_SMS4];
         CopyBlock16(pIV, IV);

      if(len/cfbBlkSize >= 4)
      {
         cpSMS4_CFB_dec_gfni512(pDst, pSrc, len, cfbBlkSize, (Ipp32u*)SMS4_RK(pCtx), IV); /* pipeline */

         int processedLen = len - (len % (4*cfbBlkSize));
         pSrc += processedLen;
         pDst += processedLen;
         len = len - processedLen;
      }
      if(len)
      {
         cpDecryptSMS4_cfb(IV, pSrc, pDst, len/cfbBlkSize, cfbBlkSize, pCtx); /* tail */
      }
   }
   else
   #endif
   #endif
   {
      cpDecryptSMS4_cfb(pIV, pSrc, pDst, len/cfbBlkSize, cfbBlkSize, pCtx);
   }

   return ippStsNoErr;
}
