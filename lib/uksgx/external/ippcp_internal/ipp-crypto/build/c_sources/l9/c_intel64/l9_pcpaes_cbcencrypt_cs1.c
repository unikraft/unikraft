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
//     AES encryption/decryption (CBC mode)
//     AES encryption/decryption (CBC-CS mode)
// 
//  Contents:
//        ippsAESEncryptCBC_CS1()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpaesm.h"
#include "pcptool.h"
#include "pcpaes_cbc_encrypt.h"

#if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
#  include "pcprijtables.h"
#endif

/*F*
//    Name: ippsAESEncryptCBC_CS1
//
// Purpose: AES-CBC-CS1 encryption.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pCtx == NULL
//                            pSrc == NULL
//                            pDst == NULL
//                            pIV  == NULL
//    ippStsContextMatchErr   !VALID_AES_ID()
//    ippStsLengthErr         len <MBS_RIJ128
//    ippStsNoErr             no errors
//
// Parameters:
//    pSrc        pointer to the source data buffer
//    pDst        pointer to the target data buffer
//    len         input/output buffer length (in bytes)
//    pCtx        pointer to the AES context
//    pIV         pointer to the initialization vector
//
// Note:
// - if last input block is a complete block,
//   CBC-CS1 encryption is equivalent to usual CBC encryption
// - if last input block is a incomplete block,
//   penultimate (ciphertext) block C*[n-2] = MSB(C[n-2], tail), C[n-2] = ENC(P[n-2])
//   and last (ciphertext) block C[n-1] = ENC(ZeroPad(P[n-1], tail))
*F*/
IPPFUN(IppStatus, ippsAESEncryptCBC_CS1,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                         const IppsAESSpec* pCtx,
                                         const Ipp8u* pIV))
{
   /* test context */
   IPP_BAD_PTR1_RET(pCtx);
   /* test the context ID */
   IPP_BADARG_RET(!VALID_AES_ID(pCtx), ippStsContextMatchErr);

   /* test source, target buffers and initialization pointers */
   IPP_BAD_PTR3_RET(pSrc, pIV, pDst);
   /* test stream length */
   IPP_BADARG_RET((len<MBS_RIJ128), ippStsLengthErr);

   {
      int tail = len & (MBS_RIJ128-1); /* length of the last partial block */
      len -= tail;

      /* encryption of complete blocks */
      cpEncryptAES_cbc(pIV, pSrc, pDst, len/MBS_RIJ128, pCtx);
      pSrc += len;
      pDst += len;

      if(tail) {
         RijnCipher encoder = RIJ_ENCODER(pCtx);
         Ipp8u lastIV[MBS_RIJ128];
         int n;

         CopyBlock16(pDst-MBS_RIJ128, lastIV);
         for(n=0; n<tail; n++)
            lastIV[n] ^= pSrc[n];

         /* encrypt last padded block */
         #if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
         encoder(lastIV, pDst-MBS_RIJ128+tail, RIJ_NR(pCtx), RIJ_EKEYS(pCtx), RijEncSbox/*NULL*/);
         #else
         encoder(lastIV, pDst-MBS_RIJ128+tail, RIJ_NR(pCtx), RIJ_EKEYS(pCtx), NULL);
         #endif
      }

      return ippStsNoErr;
   }
}
