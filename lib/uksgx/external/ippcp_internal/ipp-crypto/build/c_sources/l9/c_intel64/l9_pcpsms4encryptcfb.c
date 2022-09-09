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
//        ippsSMS4EncryptCFB()
//
*/

#include "owncp.h"
#include "pcpsms4.h"
#include "pcptool.h"

/*F*
//    Name: ippsSMS4EncryptCFB
//
// Purpose: SMS4-CFB encryption.
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
IPPFUN(IppStatus, ippsSMS4EncryptCFB,(const Ipp8u* pSrc, Ipp8u* pDst, int len, int cfbBlkSize,
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

   {
      __ALIGN16 Ipp32u TMP[3*MBS_SMS4];

      /*
         tmpInp     size = 2*MBS_SMS4
         tmpOut     size = MBS_SMS4
      */

      Ipp32u*     tmpInp = TMP;
      Ipp32u*     tmpOut = TMP + 2*MBS_SMS4;

      /* read IV */
      CopyBlock16(pIV, tmpInp);

      /* encrypt data block-by-block of cfbLen each */
      while(len>=cfbBlkSize) {
         int n;

         /* encryption */
         cpSMS4_Cipher((Ipp8u*)tmpOut, (Ipp8u*)tmpInp, SMS4_RK(pCtx));

         /* store output and put feedback into the input buffer (tmpInp) */
         if( cfbBlkSize==MBS_SMS4 && pSrc!=pDst) {
            tmpInp[0] = ((Ipp32u*)pDst)[0] = tmpOut[0]^((Ipp32u*)pSrc)[0];
            tmpInp[1] = ((Ipp32u*)pDst)[1] = tmpOut[1]^((Ipp32u*)pSrc)[1];
            tmpInp[2] = ((Ipp32u*)pDst)[2] = tmpOut[2]^((Ipp32u*)pSrc)[2];
            tmpInp[3] = ((Ipp32u*)pDst)[3] = tmpOut[3]^((Ipp32u*)pSrc)[3];
         }
         else {
            for(n=0; n<cfbBlkSize; n++) {
               pDst[n] = (Ipp8u)( ((Ipp8u*)tmpOut)[n] ^ pSrc[n] );
               ((Ipp8u*)tmpInp)[MBS_SMS4+n] = pDst[n];
            }

            /* shift input buffer (tmpInp) for the next CFB operation */
            CopyBlock16((Ipp8u*)tmpInp+cfbBlkSize, tmpInp);
         }

         pSrc += cfbBlkSize;
         pDst += cfbBlkSize;
         len -= cfbBlkSize;
      }

      /* clear secret data */
      PurgeBlock(TMP, sizeof(TMP));

      return ippStsNoErr;
   }
}
