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
//     RSASSA-PSS
// 
//     Signatire Scheme with Appendix Signatute Generation
//     (Ppobabilistic Signature Scheme)
// 
//  Contents:
//        ippsRSASign_PSS()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpngrsa.h"
#include "pcphash.h"
#include "pcptool.h"

/*F*
// Name: ippsRSASign_PSS
//
// Purpose: Performs Signature Generation according to RSASSA-PSS
//
// Returns:                   Reason:
//    ippStsNotSupportedModeErr  invalid hashAlg value
//
//    ippStsNullPtrErr           NULL == pMsg
//                               NULL == pSalt
//                               NULL == pSign
//                               NULL == pPrvKey
//                               NULL == pPubKey
//                               NULL == pBuffer
//
//    ippStsLengthErr            msgLen<0
//                               saltLen<0
//                               emLen < (hashLen +saltLen +2),
//                                  where emLen = (BITSIZE(RSA)-1)/8
//
//    ippStsContextMatchErr      !RSA_PRV_KEY_VALID_ID()
//                               !RSA_PUB_KEY_VALID_ID()
//
//    ippStsIncompleteContextErr private or/and public key is not set up
//
//    ippStsNoErr                no error
//
// Parameters:
//    pMsg        pointer to the message to be signed
//    msgLen      lenfth of the message
//    pSalt       "salt" pointer to random string
//    saltLen     length of the "salt" string (bytes)
//    pSign       pointer to the signature string of the RSA length
//    pPrvKey     pointer to the RSA private key context
//    pPubKey     (optional) pointer to the RSA public key context
//    hashAlg     hash ID
//    pBuffer     pointer to scratch buffer
*F*/
IPPFUN(IppStatus, ippsRSASign_PSS,(const Ipp8u* pMsg,  int msgLen,
                                   const Ipp8u* pSalt, int saltLen,
                                         Ipp8u* pSign,
                                   const IppsRSAPrivateKeyState* pPrvKey,
                                   const IppsRSAPublicKeyState*  pPubKey,
                                         IppHashAlgId hashAlg,
                                         Ipp8u* pScratchBuffer))
{
   /* test hash algorith ID */
   hashAlg = cpValidHashAlg(hashAlg);
   IPP_BADARG_RET(ippHashAlg_Unknown==hashAlg, ippStsNotSupportedModeErr);

   /* test message length */
   IPP_BADARG_RET((msgLen<0), ippStsLengthErr);
   /* test message pointer */
   IPP_BADARG_RET((msgLen && !pMsg), ippStsNullPtrErr);

   /* test data pointer */
   IPP_BAD_PTR1_RET(pSign);

   /* test salt length and salt pointer */
   IPP_BADARG_RET(saltLen<0, ippStsLengthErr);
   IPP_BADARG_RET((saltLen && !pSalt), ippStsNullPtrErr);

   /* test private key context */
   IPP_BAD_PTR2_RET(pPrvKey, pScratchBuffer);
   IPP_BADARG_RET(!RSA_PRV_KEY_VALID_ID(pPrvKey), ippStsContextMatchErr);
   IPP_BADARG_RET(!RSA_PRV_KEY_IS_SET(pPrvKey), ippStsIncompleteContextErr);

   /* use public key context if defined */
   if(pPubKey) {
      IPP_BADARG_RET(!RSA_PUB_KEY_VALID_ID(pPubKey), ippStsContextMatchErr);
      IPP_BADARG_RET(!RSA_PUB_KEY_IS_SET(pPubKey), ippStsIncompleteContextErr);
   }

   {
      Ipp8u hashMsg[MAX_HASH_SIZE];

      /* hash length */
      int hashLen = cpHashSize(hashAlg);

      /* size of RSA modulus in bytes and chunks */
      cpSize rsaBits = RSA_PRV_KEY_BITSIZE_N(pPrvKey);
      cpSize k = BITS2WORD8_SIZE(rsaBits);
      cpSize nsN = BITS_BNU_CHUNK(rsaBits);

      /* align buffer */
      BNU_CHUNK_T* pBuffer = (BNU_CHUNK_T*)(IPP_ALIGNED_PTR(pScratchBuffer, (int)sizeof(BNU_CHUNK_T)) );

      /* temporary BNs */
      __ALIGN8 IppsBigNumState bnC;
      __ALIGN8 IppsBigNumState bnP;

      /* message presentative size */
      int emBits = rsaBits-1;
      int emLen  = BITS2WORD8_SIZE(emBits);

      /* size of padding string (PS) */
      int psLen = emLen -hashLen -saltLen -2;

      /* test size consistence */
      if(0 > psLen)
         IPP_ERROR_RET(ippStsLengthErr);

      /* compute hash of the message */
      ippsHashMessage(pMsg, msgLen, hashMsg, hashAlg);

      /* make BNs */
      BN_Make(pBuffer, pBuffer+nsN+1, nsN, &bnC);
      pBuffer += (nsN+1)*2;
      BN_Make(pBuffer, pBuffer+nsN+1, nsN, &bnP);
      pBuffer += (nsN+1)*2;

      /*
      // EMSA-PSS encoding
      */
      {
         Ipp8u* pM  = (Ipp8u*)BN_NUMBER(&bnP);
         Ipp8u* pEM = pSign;
         Ipp8u* pDB = pSign;
         int dbLen = emLen-hashLen-1;
         Ipp8u* pH  = pSign+dbLen;

         /* construct message M'
         // M' = (00 00 00 00 00 00 00 00) || mHash || salt
         // where:
         //    mHash = HASH(pMsg)
         */
         PadBlock(0, pM, 8);
         CopyBlock(hashMsg, pM+8, hashLen);
         CopyBlock(pSalt, pM+8+hashLen, saltLen);

         /* construct EM
         // EM = maskedDB || H || 0xBC
         // where:
         //    H = HASH(M')
         //    maskedDB = DB ^ MGF(H)
         //    where:
         //       DB = PS || 0x01 || salt
         //
         // by other words
         // EM = (dbMask ^ (PS || 0x01 || salt)) || HASH(M) || 0xBC
         */
         pEM[emLen-1] = 0xBC;                               /* tail octet */
         ippsHashMessage(pM, 8+hashLen+saltLen, pH, hashAlg); /* H = HASH(M) */
         ippsMGF(pH, hashLen, pDB, dbLen, hashAlg);           /* dbMask = MGF(H) */

         XorBlock(pDB+psLen+1, pSalt, pDB+psLen+1, saltLen);
         pDB[psLen] ^= 0x01;

         /* make sure that top 8*emLen-emBits bits are clear */
         pDB[0] &= MAKEMASK32(8-8*emLen+emBits);
      }

      /*
      // private-key operation
      */
      ippsSetOctString_BN(pSign, emLen, &bnC);

      if(RSA_PRV_KEY1_VALID_ID(pPrvKey))
         gsRSAprv_cipher(&bnP, &bnC, pPrvKey, pBuffer);
      else
         gsRSAprv_cipher_crt(&bnP, &bnC, pPrvKey, pBuffer);

      ippsGetOctString_BN(pSign, k, &bnP);

      /* no check requested */
      if(!pPubKey)
         return ippStsNoErr;

      /* check the result before send it out (fault attack mitigatioin) */
      else {
         gsRSApub_cipher(&bnP, &bnP, pPubKey, pBuffer);
         if(0==cpBN_cmp(&bnP, &bnC))
            return ippStsNoErr;
         /* discard signature if check failed */
         else {
            PadBlock(0, pSign, k);
            return ippStsErr;
         }
      }
   }
}
