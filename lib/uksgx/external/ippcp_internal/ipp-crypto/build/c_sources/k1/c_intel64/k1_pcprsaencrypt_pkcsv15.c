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
//     RSA-PKCS1-v1_5 Encryption Scheme
// 
//  Contents:
//        ippsRSAEncrypt_PKCSv15()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpngrsa.h"
#include "pcptool.h"


static int EncodeEME_PKCSv15(const Ipp8u*   msg, Ipp32u msgLen,
                             const Ipp8u* rndPS,
                                   Ipp8u*   pEM, Ipp32u lenEM)
{
   /*
   // encoded message format:
   //    EM = 00 || 02 || PS || 00 || Msg
   //    len(PS) >= 8
   */
   Ipp32u psLen = lenEM - msgLen - 3;

   pEM[0] = 0x00;
   pEM[1] = 0x02;
   if(rndPS)
      CopyBlock(rndPS, pEM+2, (cpSize)psLen);
   else
      PadBlock(0xFF, pEM+2, (cpSize)psLen);
   pEM[2+psLen] = 0x00;
   CopyBlock(msg, pEM+3+psLen, (cpSize)msgLen);
   return 1;
}


static int Encryption(const Ipp8u* pMsg,  int msgLen,
                      const Ipp8u* pRndPS,
                            Ipp8u* pCipherTxt,
                      const IppsRSAPublicKeyState* pKey,
                            BNU_CHUNK_T* pBuffer)
{
   /* size of RSA modulus in bytes and chunks */
   int k= BITS2WORD8_SIZE(RSA_PUB_KEY_BITSIZE_N(pKey));
   cpSize nsN = BITS_BNU_CHUNK(RSA_PUB_KEY_BITSIZE_N(pKey));

   if( (msgLen+11)<=k ) {
      /* temporary BN */
      __ALIGN8 IppsBigNumState tmpBN;
      BN_Make(pBuffer, pBuffer+nsN, nsN, &tmpBN);

      /* EME-PKCS-v1_5 encoding */
      EncodeEME_PKCSv15(pMsg, (Ipp32u)msgLen, pRndPS, (Ipp8u*)(BN_BUFFER(&tmpBN)), (Ipp32u)k);
      /*
      // public-key operation
      */
      ippsSetOctString_BN((Ipp8u*)(BN_BUFFER(&tmpBN)), k, &tmpBN);
      gsRSApub_cipher(&tmpBN, &tmpBN, pKey, pBuffer+nsN*2);

      /* convert into the cipher text */
      ippsGetOctString_BN(pCipherTxt, k, &tmpBN);
      return 1;
   }
   else
      return 0;
}

/*F*
// Name: ippsRSAEncrypt_PKCSv15
//
// Purpose: Performs Encrption according to RSA-ES-PKCS1_v1.5
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pSrc
//                               NULL == pDst
//                               NULL == pKey
//                               NULL == pBuffer
//
//    ippStsContextMatchErr      !RSA_PUB_KEY_VALID_ID()
//
//    ippStsIncompleteContextErr public key is not set up
//
//    //ippStsLengthErr            0 == srcLen
//    ippStsSizeErr              RSA modulus too short (srcLen > k-11, see PKCS#1) !!runtime error
//    ippStsNoErr                no error
//
// Parameters:
//    pSrc        pointer to the plaintext to be encrypted
//    srcLen      plaintext length (bytes)
//    pRandPS     pointer to the random nonzero string of suitable length (psLen >= k -3 -srcLen)
//    pDst        pointer to the ciphertext (k bytes length, == length of RSA modulus
//    pKey        pointer to the public key context context
//    pBuffer     pointer to scratch buffer
*F*/
IPPFUN(IppStatus, ippsRSAEncrypt_PKCSv15,(const Ipp8u* pSrc, int srcLen,
                                          const Ipp8u* pRndPS,
                                                Ipp8u* pDst,
                                          const IppsRSAPublicKeyState* pKey,
                                                Ipp8u* pScratchBuffer))
{
   /* test public key context */
   IPP_BAD_PTR2_RET(pKey, pScratchBuffer);
   IPP_BADARG_RET(!RSA_PUB_KEY_VALID_ID(pKey), ippStsContextMatchErr);
   IPP_BADARG_RET(!RSA_PUB_KEY_IS_SET(pKey), ippStsIncompleteContextErr);

   /* test data pointer */
   IPP_BAD_PTR2_RET(pSrc, pDst);

   {
      BNU_CHUNK_T* pBuffer = (BNU_CHUNK_T*)(IPP_ALIGNED_PTR((pScratchBuffer), (int)sizeof(BNU_CHUNK_T)));
      return Encryption(pSrc, srcLen, pRndPS, pDst,
                        pKey,
                        pBuffer)? ippStsNoErr : ippStsSizeErr;
   }
}
