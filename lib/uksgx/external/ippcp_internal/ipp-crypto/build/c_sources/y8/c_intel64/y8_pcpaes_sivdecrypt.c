/*******************************************************************************
* Copyright 2015-2021 Intel Corporation
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
//     AES-SIV Functions (RFC 5297)
// 
//  Contents:
//        ippsAES_SIVDecrypt()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpcmac.h"
#include "pcpaesm.h"
#include "pcptool.h"
#include "pcpaes_sivstuff.h"

/*F*
//    Name: ippsAES_SIVDecrypt
//
// Purpose: RFC 5297 authenticated decryption
//
// Returns:                Reason:
//    ippStsNullPtrErr        pSrc==NULL, pDst==NULL
//                            pAuthPassed==NULL
//                            pAuthKey==NULL, pConfKey==NULL
//                            pAD== NULL, pADlen==NULL
//                            pADlen[i]!=0 && pAD[i]==0
//                            pSIV == NULL
//    ippStsLengthErr         keyLen != 16
//                            keyLen != 24
//                            keyLen != 32
//                            pADlen[i]<0
//                            numAD<0
//                            len<=0
//    ippStsNoErr             no errors
//
// Parameters:
//    pSrc     pointer to ciphertext
//    pDst     pointer to plaintext
//    len      length (in bytes) of plaintext/ciphertext
//    pAuthPassed "authentication passed" flag
//    pAuthKey pointer to the authentication key
//    pConfKey pointer to the confidendat key
//    keyLen   length of keys
//    pAD[]    array of pointers to input strings
//    pADlen[] array of input string lengths
//    numAD    number of pAD[] and pADlen[] terms
//    pSIV     pointer to input SIV
//
*F*/
IPPFUN(IppStatus, ippsAES_SIVDecrypt,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                      int* pAuthPassed,
                                      const Ipp8u* pAuthKey, const Ipp8u* pConfKey, int keyLen,
                                      const Ipp8u* pAD[], const int pADlen[], int numAD,
                                      const Ipp8u* pSIV))
{
   /* test ciphertext, plaintex and length */
   IPP_BAD_PTR2_RET(pSrc, pDst);
   IPP_BADARG_RET(0>=len, ippStsLengthErr);

   /* test keys & keyLen */
   IPP_BAD_PTR2_RET(pAuthKey, pConfKey);
   IPP_BADARG_RET(keyLen!=16 && keyLen!=24 && keyLen!=32, ippStsLengthErr);

   /* test passed flag & auth vector */
   IPP_BAD_PTR2_RET(pAuthPassed, pSIV);

   /* test arrays of input AD[] */
   IPP_BAD_PTR2_RET(pAD, pADlen);
   IPP_BADARG_RET(0>numAD, ippStsLengthErr);

   {
      int n;
      for (n = 0; n < numAD; n++) {
         /* test input message and it's length */
         IPP_BADARG_RET((pADlen[n] < 0), ippStsLengthErr);
         /* test source pointer */
         IPP_BADARG_RET((pADlen[n] && !pAD[n]), ippStsNullPtrErr);
      }
   }

   {
      int n;

      /* iv an dmask */
      Ipp8u iv[MBS_RIJ128];
      Ipp8u vmask[MBS_RIJ128] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
                                 0x7f,0xff,0xff,0xff,0x7f,0xff,0xff,0xff};
      /* AES context */
      Ipp8u aesBlob[sizeof(IppsAESSpec)];
      IppsAESSpec* paesCtx = (IppsAESSpec*)aesBlob;

      ippsAESInit(pConfKey, keyLen, paesCtx, sizeof(aesBlob));

      /* construct iv */
      for(n=0; n<MBS_RIJ128; n++) iv[n] = pSIV[n] & vmask[n];

      /* perform AES-CTR decryption */
      ippsAESDecryptCTR(pSrc, pDst, len, paesCtx, iv, BITSIZE(iv));

      PurgeBlock(&aesBlob, sizeof(aesBlob));

      /* re-compute SIV */
      {
         Ipp8u ctxBlob[sizeof(IppsAES_CMACState)];
         IppsAES_CMACState* pCtx = (IppsAES_CMACState*)ctxBlob;
         cpAES_S2V_init(iv, pAuthKey, keyLen, pCtx, sizeof(ctxBlob));

         for(n=0; n<numAD; n++) {
            cpAES_S2V_update(iv, pAD[n], pADlen[n], pCtx);
         }
         cpAES_S2V_final(iv, pDst, len, pCtx);

         PurgeBlock(&ctxBlob, sizeof(ctxBlob));
      }

      /* test */
      *pAuthPassed = EquBlock(pSIV, iv, MBS_RIJ128);
      return ippStsNoErr;
   }
}
