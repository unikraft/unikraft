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
//        ippsSMS4EncryptCBC_CS1()
//
*/

#include "owncp.h"
#include "pcpsms4.h"
#include "pcptool.h"
#include "pcpsms4_encrypt_cbc.h"

/*F*
//    Name: ippsSMS4EncryptCBC_CS1
//
// Purpose: SMS4-CBC-CS1 encryption.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pCtx == NULL
//                            pSrc == NULL
//                            pDst == NULL
//                            pIV  == NULL
//    ippStsContextMatchErr   !VALID_SMS4_ID()
//    ippStsLengthErr         len <MBS_SMS4
//    ippStsNoErr             no errors
//
// Parameters:
//    pSrc        pointer to the source data buffer
//    pDst        pointer to the target data buffer
//    len         input/output buffer length (in bytes)
//    pCtx        pointer to the SMS4 context
//    pIV         pointer to the initialization vector
//
// Note:
// - if last input block is a complete block,
//   CBC-CS1 encryption is equivalent to usual CBC encryption
// - if last input block is a incomplete block,
//   penultimate (ciphertext) block C*[n-2] = MSB(C[n-2], tail), C[n-2] = ENC(P[n-2])
//   and last (ciphertext) block C[n-1] = ENC(ZeroPad(P[n-1], tail))
*F*/
IPPFUN(IppStatus, ippsSMS4EncryptCBC_CS1,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
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
   IPP_BADARG_RET((len<MBS_SMS4), ippStsLengthErr);

   {
      int tail = len & (MBS_SMS4-1); /* length of the last partial block */
      len -= tail;

      /* encryption of complete blocks */
      cpEncryptSMS4_cbc(pIV, pSrc, pDst, len, pCtx);
      pSrc += len;
      pDst += len;

      if(tail) {
         Ipp8u lastIV[MBS_SMS4];
         int n;

         CopyBlock16(pDst-MBS_SMS4, lastIV);
         for(n=0; n<tail; n++)
            lastIV[n] ^= pSrc[n];

         /* encrypt last padded block */
         cpSMS4_Cipher(pDst-MBS_SMS4+tail, lastIV, SMS4_RK(pCtx));

         /* clear secret data */
         PurgeBlock(lastIV, sizeof(lastIV));
      }

      return ippStsNoErr;
   }
}
