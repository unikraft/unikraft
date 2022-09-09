/*******************************************************************************
* Copyright 2004-2021 Intel Corporation
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
//     PRNG Functions
// 
//  Contents:
//        cpPRNGen()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcphash.h"
#include "pcpprng.h"
#include "pcptool.h"


/*
// G() function based on SHA1
//
// Parameters:
//    T           160 bit parameter
//    pHexStr     input hex string
//    hexStrLen   size of hex string (Ipp8u segnments)
//    xBNU        160 bit BNU result
//
// Note 1:
//    must to be hexStrLen <= 64 (512 bits)
*/
static void SHA1_G(Ipp32u* xBNU, const Ipp32u* T, Ipp8u* pHexStr, int hexStrLen)
{
   /* select processing function */
   cpHashProc updateFunc;
   #if (_SHA_NI_ENABLING_==_FEATURE_ON_)
   updateFunc = UpdateSHA1ni;
   #elif (_SHA_NI_ENABLING_==_FEATURE_TICKTOCK_)
   updateFunc = IsFeatureEnabled(ippCPUID_SHA)? UpdateSHA1ni : UpdateSHA1;
   #else
   updateFunc = UpdateSHA1;
   #endif

   /* pad HexString zeros */
   PadBlock(0, pHexStr+hexStrLen, BITS2WORD8_SIZE(MAX_XKEY_SIZE)-hexStrLen);

   /* reset initial HASH value */
   xBNU[0] = T[0];
   xBNU[1] = T[1];
   xBNU[2] = T[2];
   xBNU[3] = T[3];
   xBNU[4] = T[4];

   /* SHA1 */
   //UpdateSHA1(xBNU, pHexStr, BITS2WORD8_SIZE(MAX_XKEY_SIZE), SHA1_cnt);
   updateFunc(xBNU, pHexStr, BITS2WORD8_SIZE(MAX_XKEY_SIZE), SHA1_cnt);

   /* swap back */
   SWAP(xBNU[0],xBNU[4]);
   SWAP(xBNU[1],xBNU[3]);
}

/*F*
// Name: cpPRNGen
//
// Purpose: Returns bitsize of the bitstring has beed added
//
// Returns:
//    bitsize of the bitstring has beed added
//
// Parameters:
//    pRand    pointer to the buffer
//    nBits    number of bits be requested
//    pRnd     pointer to the context
*F*/

IPP_OWN_DEFN (int, cpPRNGen, (Ipp32u* pRand, cpSize nBits, IppsPRNGState* pRnd))
{
   BNU_CHUNK_T Xj  [BITS_BNU_CHUNK(MAX_XKEY_SIZE)];
   BNU_CHUNK_T XVAL[BITS_BNU_CHUNK(MAX_XKEY_SIZE)];

   Ipp8u  TXVAL[BITS2WORD8_SIZE(MAX_XKEY_SIZE)];

   /* XKEY length in BNU_CHUNK_T */
   cpSize xKeyLen = BITS_BNU_CHUNK(RAND_SEEDBITS(pRnd));
   /* XKEY length in bytes */
   cpSize xKeySize= BITS2WORD8_SIZE(RAND_SEEDBITS(pRnd));
   /* XKEY word's mask */
   BNU_CHUNK_T xKeyMsk = MASK_BNU_CHUNK(RAND_SEEDBITS(pRnd));

   /* number of Ipp32u chunks to be generated */
   cpSize genlen = BITS2WORD32_SIZE(nBits);

   ZEXPAND_BNU(Xj, 0, BITS_BNU_CHUNK(MAX_XKEY_SIZE));
   ZEXPAND_BNU(XVAL, 0, BITS_BNU_CHUNK(MAX_XKEY_SIZE));

   while(genlen) {
      cpSize len;

      /* Step 1: XVAL=(Xkey+Xseed) mod 2^b */
      BNU_CHUNK_T carry = cpAdd_BNU(XVAL, RAND_XKEY(pRnd), RAND_XAUGMENT(pRnd), xKeyLen);
      XVAL[xKeyLen-1] &= xKeyMsk;

      /* Step 2: xj=G(t, XVAL) mod Q */
      cpToOctStr_BNU(TXVAL, xKeySize, XVAL, xKeyLen);
      SHA1_G((Ipp32u*)Xj, (Ipp32u*)RAND_T(pRnd), TXVAL, xKeySize);

      {
         cpSize sizeXj = BITS_BNU_CHUNK(160);
         if(0 <= cpCmp_BNU(Xj, BITS_BNU_CHUNK(IPP_SHA1_DIGEST_BITSIZE), RAND_Q(pRnd),BITS_BNU_CHUNK(IPP_SHA1_DIGEST_BITSIZE)) )
            sizeXj = cpMod_BNU(Xj, BITS_BNU_CHUNK(IPP_SHA1_DIGEST_BITSIZE), RAND_Q(pRnd), BITS_BNU_CHUNK(IPP_SHA1_DIGEST_BITSIZE));
         FIX_BNU(Xj, sizeXj);
         ZEXPAND_BNU(Xj, sizeXj, BITS_BNU_CHUNK(MAX_XKEY_SIZE));
      }

      /* Step 3: Xkey=(1+Xkey+Xj) mod 2^b */
      cpInc_BNU(RAND_XKEY(pRnd), RAND_XKEY(pRnd), xKeyLen, 1);
      carry = cpAdd_BNU(RAND_XKEY(pRnd), RAND_XKEY(pRnd), Xj, xKeyLen);
      RAND_XKEY(pRnd)[xKeyLen-1] &= xKeyMsk;

      /* fill out result */
      len = genlen<BITS2WORD32_SIZE(IPP_SHA1_DIGEST_BITSIZE)? genlen : BITS2WORD32_SIZE(IPP_SHA1_DIGEST_BITSIZE);
      COPY_BNU(pRand, (Ipp32u*)Xj, len);

      pRand  += len;
      genlen -= len;
   }

   return nBits;
}
