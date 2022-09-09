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
//     RSAES-OAEP Encryption/Decription Functions
// 
//  Contents:
//        ippsRSAEncrypt_OAEP()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcptool.h"
#include "pcpngrsa.h"
#include "pcphash.h"

/*F*
// Name: ippsRSAEncrypt_OAEP
//
// Purpose: Performs RSAES-OAEP encryprion scheme
//
// Returns:                   Reason:
//    ippStsNotSupportedModeErr  unknown hashAlg
//
//    ippStsNullPtrErr           NULL == pKey
//                               NULL == pSrc
//                               NULL == pDst
//                               NULL == pLabel
//                               NULL == pSeed
//                               NULL == pBuffer
//
//    ippStsLengthErr            srcLen <0
//                               labLen <0
//                               srcLen > RSAsize -2*hashLen -2
//                               RSAsize < 2*hashLen +2
//
//    ippStsContextMatchErr      !RSA_PUB_KEY_VALID_ID()
//
//    ippStsIncompleteContextErr public key is not set up
//
//    ippStsNoErr                no error
//
// Parameters:
//    pSrc        pointer to the plaintext
//    srcLen      plaintext length (bytes)
//    pLabel      (optional) pointer to the label associated with plaintext
//    labLen      label length (bytes)
//    pSeed       seed string of hashLen size
//    pDst        pointer to the ciphertext (length of pdst is not less then size of RSA modulus)
//    pKey        pointer to the RSA public key context
//    hashAlg     hash alg ID
//    pBuffer     pointer to scratch buffer
*F*/
IPPFUN(IppStatus, ippsRSAEncrypt_OAEP,(const Ipp8u* pSrc, int srcLen,
                                       const Ipp8u* pLabel, int labLen, 
                                       const Ipp8u* pSeed,
                                             Ipp8u* pDst,
                                       const IppsRSAPublicKeyState* pKey,
                                             IppHashAlgId hashAlg,
                                             Ipp8u* pScratchBuffer))
{
   int hashLen;

   /* test hash algorith ID */
   hashAlg = cpValidHashAlg(hashAlg);
   IPP_BADARG_RET(ippHashAlg_Unknown==hashAlg, ippStsNotSupportedModeErr);

   /* test data pointer */
   IPP_BAD_PTR3_RET(pSrc,pDst, pSeed);

   IPP_BADARG_RET(!pLabel && labLen, ippStsNullPtrErr);

   /* test public key context */
   IPP_BAD_PTR2_RET(pKey, pScratchBuffer);
   IPP_BADARG_RET(!RSA_PUB_KEY_VALID_ID(pKey), ippStsContextMatchErr);
   IPP_BADARG_RET(!RSA_PUB_KEY_IS_SET(pKey), ippStsIncompleteContextErr);

   /* test length */
   IPP_BADARG_RET(srcLen<0||labLen<0, ippStsLengthErr);

    hashLen = cpHashSize(hashAlg);
   /* test compatibility of RSA and hash length */
   IPP_BADARG_RET(BITS2WORD8_SIZE(RSA_PRV_KEY_BITSIZE_N(pKey)) < (2*hashLen +2), ippStsLengthErr);
   /* test compatibility of msg length and other (RSA and hash) lengths */
   IPP_BADARG_RET(BITS2WORD8_SIZE(RSA_PRV_KEY_BITSIZE_N(pKey))-(2*hashLen +2) < srcLen, ippStsLengthErr);

   {
      /* size of RSA modulus in bytes and chunks */
      int k = BITS2WORD8_SIZE(RSA_PUB_KEY_BITSIZE_N(pKey));
      cpSize nsN = BITS_BNU_CHUNK(RSA_PUB_KEY_BITSIZE_N(pKey));

      /*
      // EME-OAEP encoding
      */
      {
         Ipp8u  seedMask[BITS2WORD8_SIZE(IPP_SHA512_DIGEST_BITSIZE)];

         Ipp8u* pMaskedSeed = pDst+1;
         Ipp8u* pMaskedDB = pDst +hashLen +1;

         pDst[0] = 0;

         /* maskedDB = MGF(seed, k-1-hashLen)*/
         ippsMGF(pSeed, hashLen, pMaskedDB, k-1-hashLen, hashAlg);

         /* seedMask = HASH(pLab) */
         ippsHashMessage(pLabel, labLen, seedMask, hashAlg);

         /* maskedDB ^= concat(HASH(pLab),PS,0x01,pSc) */
         XorBlock(pMaskedDB, seedMask, pMaskedDB, hashLen);
         pMaskedDB[k-srcLen-hashLen-2] ^= 0x01;
         XorBlock(pMaskedDB+k-srcLen-hashLen-2+1, pSrc, pMaskedDB+k-srcLen-hashLen-2+1, srcLen);

         /* seedMask = MGF(maskedDB, hashLen) */
         ippsMGF(pMaskedDB, k-1-hashLen, seedMask, hashLen, hashAlg);
         /* maskedSeed = seed ^ seedMask */
         XorBlock(pSeed, seedMask, pMaskedSeed, hashLen);
      }

      /* RSA encryption */
      {
         /* align buffer */
         BNU_CHUNK_T* pBuffer = (BNU_CHUNK_T*)(IPP_ALIGNED_PTR(pScratchBuffer, (int)sizeof(BNU_CHUNK_T)) );

         /* temporary BN */
         __ALIGN8 IppsBigNumState tmpBN;
         BN_Make(pBuffer, pBuffer+nsN+1, nsN, &tmpBN);

         /* updtae buffer pointer */
         pBuffer += (nsN+1)*2;

         ippsSetOctString_BN(pDst, k, &tmpBN);

         gsRSApub_cipher(&tmpBN, &tmpBN, pKey, pBuffer);

         ippsGetOctString_BN(pDst, k, &tmpBN);
      }

      return ippStsNoErr;
   }
}
