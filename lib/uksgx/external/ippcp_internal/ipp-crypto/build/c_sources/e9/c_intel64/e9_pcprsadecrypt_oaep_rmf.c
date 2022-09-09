/*******************************************************************************
* Copyright 2016-2021 Intel Corporation
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
//     RSAES-OAEP Encryption/Decription Functions
//
//  Contents:
//        ippsRSADecrypt_OAEP_rmf()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcptool.h"
#include "pcpngrsa.h"
#include "pcphash_rmf.h"

/*F*
// Name: ippsRSADecrypt_OAEP_rmf
//
// Purpose: Performs RSAES-OAEP decryprion scheme
//
// Returns:                   Reason:
//    ippStsNotSupportedModeErr  unknown hashAlg
//
//    ippStsNullPtrErr           NULL == pKey
//                               NULL == pSrc
//                               NULL == pLab
//                               NULL == pDst
//                               NULL == pDstLen
//                               NULL == pMethod
//                               NULL == pBuffer
//
//    ippStsLengthErr            labLen <0
//                               RSAsize < 2*hashLen +2
//
//    ippStsIncompleteContextErr private key is not set up
//
//    ippStsContextMatchErr      !RSA_PRV_KEY_VALID_ID()
//    ippStsNoErr                no error
//
// Parameters:
//    pSrc        pointer to the ciphertext
//    pLab        (optional) pointer to the label associated with plaintext
//    labLen      label length (bytes)
//    pDst        pointer to the plaintext
//    pDstLen     pointer to the plaintext length
//    pKey        pointer to the RSA private key context
//    pMethod     hash methods
//    pBuffer     pointer to scratch buffer
*F*/
IPPFUN(IppStatus, ippsRSADecrypt_OAEP_rmf,(const Ipp8u* pSrc,
                                           const Ipp8u* pLab, int labLen,
                                                 Ipp8u* pDst, int* pDstLen,
                                           const IppsRSAPrivateKeyState* pKey,
                                           const IppsHashMethod* pMethod,
                                             Ipp8u* pScratchBuffer))
{
   int hashLen;

   /* test data pointer */
   IPP_BAD_PTR4_RET(pSrc, pDst, pDstLen, pMethod);

   IPP_BADARG_RET(!pLab && labLen, ippStsNullPtrErr);

   IPP_BAD_PTR2_RET(pKey, pScratchBuffer);
   IPP_BADARG_RET(!RSA_PRV_KEY_VALID_ID(pKey), ippStsContextMatchErr);
   IPP_BADARG_RET(!RSA_PRV_KEY_IS_SET(pKey), ippStsIncompleteContextErr);

   /* test hash length */
   IPP_BADARG_RET(labLen<0, ippStsLengthErr);

    hashLen = pMethod->hashLen;
   /* test compatibility of RSA and hash length */
   IPP_BADARG_RET(BITS2WORD8_SIZE(RSA_PRV_KEY_BITSIZE_N(pKey)) < (2*hashLen +2), ippStsLengthErr);

   {
      /* size of RSA modulus in bytes and chunks */
      int k = BITS2WORD8_SIZE(RSA_PRV_KEY_BITSIZE_N(pKey));
      cpSize nsN = BITS_BNU_CHUNK(RSA_PRV_KEY_BITSIZE_N(pKey));

      /* align buffer */
      BNU_CHUNK_T* pBuffer = (BNU_CHUNK_T*)(IPP_ALIGNED_PTR(pScratchBuffer, (int)sizeof(BNU_CHUNK_T)) );

      /* temporary BN */
      __ALIGN8 IppsBigNumState tmpBN;
      BN_Make(pBuffer, pBuffer+nsN+1, nsN, &tmpBN);

      /* update buffer pointer */
      pBuffer += (nsN+1)*2;

      /* RSA decryption */
      ippsSetOctString_BN(pSrc, k, &tmpBN);

      if(RSA_PRV_KEY1_VALID_ID(pKey))
         gsRSAprv_cipher(&tmpBN, &tmpBN, pKey, pBuffer);
      else
         gsRSAprv_cipher_crt(&tmpBN, &tmpBN, pKey, pBuffer);

      /*
      // EME-OAEP decoding
      */
      {
         Ipp8u  seedMask[BITS2WORD8_SIZE(IPP_SHA512_DIGEST_BITSIZE)];

         Ipp8u* pEMessg = (Ipp8u*)( BN_BUFFER(&tmpBN) );
         Ipp8u* pMaskedSeed = pEMessg+1;
         Ipp8u* pMaskedDB = pEMessg+1+hashLen;
         Ipp8u* pDB = (Ipp8u*)BN_NUMBER(&tmpBN);

         ippsGetOctString_BN(pEMessg, k, &tmpBN);

         ippsMGF1_rmf(pMaskedDB, k-1-hashLen, seedMask, hashLen, pMethod);
         XorBlock(pMaskedSeed, seedMask, pMaskedSeed, hashLen);

         ippsMGF1_rmf(pMaskedSeed, hashLen, pDB, k-1-hashLen, pMethod);
         XorBlock(pMaskedDB, pDB, pMaskedDB, k-1-hashLen);

         ippsHashMessage_rmf(pLab, labLen, seedMask, pMethod);

         *pDstLen = 0;
         /*
         // check EM
         */
         if(0==EquBlock(seedMask, pMaskedDB, hashLen))
            return ippStsUnderRunErr;
         else {
            int i;
            for(i=hashLen; i<k-1-hashLen; i++) {
               if(pMaskedDB[i])
                  break;
            }
            if(0x01 != pMaskedDB[i] )
               return ippStsUnderRunErr;

            *pDstLen = k-1-hashLen -i -1;
            CopyBlock(pMaskedDB+i+1, pDst, k-1-hashLen -i -1);

            return ippStsNoErr;
         }
      }
   }
}
