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
//        cpProcessSMS4_ofb8()
//
*/

#include "owncp.h"
#include "pcpsms4.h"
#include "pcptool.h"
#include "pcpsms4_process_ofb8.h"

/*
// SMS4-OFB ecnryption/decryption
//
// Parameters:
//    pSrc        pointer to the source data buffer
//    pDst        pointer to the target data buffer
//    dataLen     input/output buffer length (in bytes)
//    ofbBlkSize  ofb block size (in bytes)
//    pCtx        pointer to the AES context
//    pIV         pointer to the initialization vector
*/
IPP_OWN_DEFN (void, cpProcessSMS4_ofb8, (const Ipp8u *pSrc, Ipp8u *pDst, int dataLen, int ofbBlkSize, const IppsSMS4Spec* pCtx, Ipp8u* pIV))
{
   __ALIGN16 Ipp32u tmpInpOut[2*MBS_SMS4/sizeof(Ipp32u)];

   CopyBlock16(pIV, tmpInpOut);

   while(dataLen>=ofbBlkSize) {
      /* block-by-block processing */
      cpSMS4_Cipher((Ipp8u*)tmpInpOut+MBS_SMS4, (Ipp8u*)tmpInpOut, SMS4_RK(pCtx));

      /* store output and shift inpBuffer for the next OFB operation */
      if(ofbBlkSize==MBS_SMS4) {
         ((Ipp32u*)pDst)[0] = tmpInpOut[0+MBS_SMS4/sizeof(Ipp32u)]^((Ipp32u*)pSrc)[0];
         ((Ipp32u*)pDst)[1] = tmpInpOut[1+MBS_SMS4/sizeof(Ipp32u)]^((Ipp32u*)pSrc)[1];
         ((Ipp32u*)pDst)[2] = tmpInpOut[2+MBS_SMS4/sizeof(Ipp32u)]^((Ipp32u*)pSrc)[2];
         ((Ipp32u*)pDst)[3] = tmpInpOut[3+MBS_SMS4/sizeof(Ipp32u)]^((Ipp32u*)pSrc)[3];
         tmpInpOut[0] = tmpInpOut[0+MBS_SMS4/sizeof(Ipp32u)];
         tmpInpOut[1] = tmpInpOut[1+MBS_SMS4/sizeof(Ipp32u)];
         tmpInpOut[2] = tmpInpOut[2+MBS_SMS4/sizeof(Ipp32u)];
         tmpInpOut[3] = tmpInpOut[3+MBS_SMS4/sizeof(Ipp32u)];
      }
      else {
         XorBlock(pSrc, tmpInpOut+MBS_SMS4/sizeof(Ipp32u), pDst, ofbBlkSize);
         CopyBlock16((Ipp8u*)tmpInpOut+ofbBlkSize, tmpInpOut);
      }

      pSrc += ofbBlkSize;
      pDst += ofbBlkSize;
      dataLen -= ofbBlkSize;
   }

   /* update pIV */
   CopyBlock16((Ipp8u*)tmpInpOut, pIV);
   
   /* clear secret data */
   PurgeBlock(tmpInpOut, sizeof(tmpInpOut));
}
