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
//        ippsAESDecryptCBC_CS2()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpaesm.h"
#include "pcptool.h"
#include "pcpaes_cbc_decrypt.h"

#if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
#  include "pcprijtables.h"
#endif


/*F*
//    Name: ippsAESDecryptCBC_CS2
//
// Purpose: AES-CBC_CS2 decryption.
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
*F*/
IPPFUN(IppStatus, ippsAESDecryptCBC_CS2,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
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

      if(tail) {
         RijnCipher decoder = RIJ_DECODER(pCtx);

         Ipp8u lastIV[MBS_RIJ128];
         Ipp8u lastDecBlk[2*MBS_RIJ128];

         int n;

         len -= MBS_RIJ128+tail;

         if(len) {
            CopyBlock16(pSrc+len-MBS_RIJ128, lastIV);
            cpDecryptAES_cbc(pIV, pSrc, pDst, len/MBS_RIJ128, pCtx);
            pSrc += len;
            pDst += len;
         }
         else
            CopyBlock16(pIV, lastIV);

         /* decrypt last  block */
         #if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
         decoder(pSrc, lastDecBlk+MBS_RIJ128, RIJ_NR(pCtx), RIJ_EKEYS(pCtx), RijDecSbox/*NULL*/);
         #else
         decoder(pSrc, lastDecBlk+MBS_RIJ128, RIJ_NR(pCtx), RIJ_DKEYS(pCtx), NULL);
         #endif

         CopyBlock16(lastDecBlk+MBS_RIJ128, lastDecBlk);
         for(n=0; n<tail; n++) lastDecBlk[n] = pSrc[n+MBS_RIJ128];
         /* decrypt penultimate  block */
         #if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
         decoder(lastDecBlk, lastDecBlk, RIJ_NR(pCtx), RIJ_EKEYS(pCtx), RijDecSbox/*NULL*/);
         #else
         decoder(lastDecBlk, lastDecBlk, RIJ_NR(pCtx), RIJ_DKEYS(pCtx), NULL);
         #endif

         for(n=0; n<MBS_RIJ128; n++) {
            Ipp8u c = pSrc[n];
            pDst[n] = lastDecBlk[n] ^ lastIV[n];
            lastIV[n] = c;
         }
         for(tail+=MBS_RIJ128; n<tail; n++)
            pDst[n] = lastDecBlk[n] ^ pSrc[n];

         /* clear secret data */
         PurgeBlock(lastDecBlk, sizeof(lastDecBlk));
      }

      else
         cpDecryptAES_cbc(pIV, pSrc, pDst, len/MBS_RIJ128, pCtx);

      return ippStsNoErr;
   }
}
