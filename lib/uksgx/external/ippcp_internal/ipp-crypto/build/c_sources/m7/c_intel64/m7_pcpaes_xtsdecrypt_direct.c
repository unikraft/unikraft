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
// 
//  Purpose:
//     Cryptography Primitive.
//     AES-XTS Functions (IEEE P1619)
// 
//  Contents:
//        ippsAESDecryptXTS_Direct()
//
*/

#include "owncp.h"
#include "pcpaesm.h"
#include "pcptool.h"
#include "pcpaesmxtsstuff.h"

#if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
#  include "pcprijtables.h"
#endif

#if !defined AES_BLK_SIZE
#define AES_BLK_SIZE (IPP_AES_BLOCK_BITSIZE/BITSIZE(Ipp8u))
#endif

#define AES_BLKS_PER_BUFFER (32)


/*F*
//    Name: ippsAESDecryptXTS_Direct
//
// Purpose: AES-XTS decryption (see IEEE P1619-2007).
//
// Returns:                Reason:
//    ippStsNullPtrErr        pSrc == NULL
//                            pDst == NULL
//                            pTweak ==NULL
//                            pKey ==NULL
//    ippStsLengthErr         dataUnitBitsize <128
//                            keyBitsize != 256, !=512
//    ippStsBadArgErr         aesBlkNo >= dataUnitBitsize/IPP_AES_BLOCK_BITSIZE
//    ippStsNoErr             no errors
//
// Parameters:
//    pSrc              points input buffer
//    pDst              points output buffer
//    encBitsize        lendth of the input/output buffer in bits
//    aesBlkNo          number of the first block for data unit
//    ptweakPT          points tweak value
//    pKey              pointer to the XTS key
//    keyBitsize        length of the key in bits
//    dataUnitBitsize   length of Data Unit in bits
//
*F*/
static IppStatus cpAES_XTS_DecBlock(Ipp8u* ctxt, const Ipp8u* ptxt, const Ipp8u* tweak, const IppsAESSpec* pEncCtx)
{
   IppStatus sts;
   /* pre-whitening */
   XorBlock16(ptxt, tweak, ctxt);
   /* decryption */
   sts = ippsAESDecryptECB(ctxt, ctxt, AES_BLK_SIZE, pEncCtx);
   /* post-whitening */
   XorBlock16(ctxt, tweak, ctxt);
   return sts;
}

//
// To Do: advance parameter check!!
//
IPPFUN(IppStatus, ippsAESDecryptXTS_Direct,(const Ipp8u* pSrc, Ipp8u* pDst, int encBitsize, int aesBlkNo,
                                     const Ipp8u* pTweakPT,
                                     const Ipp8u* pKey, int keyBitsize,
                                           int dataUnitBitsize))
{
   /* test dataUnitBitsize */
   IPP_BADARG_RET(dataUnitBitsize<IPP_AES_BLOCK_BITSIZE, ippStsLengthErr);

   /* test key and keyBitsize */
   IPP_BAD_PTR1_RET(pKey);
   IPP_BADARG_RET(256 != keyBitsize && 512!=keyBitsize, ippStsLengthErr);

   /* test pTweak, pSrc, pDst and encBitsize */
   IPP_BAD_PTR3_RET(pTweakPT, pSrc, pDst);
   IPP_BADARG_RET(encBitsize<IPP_AES_BLOCK_BITSIZE, ippStsLengthErr);

   /* impractical case, but test input length (shall not exceed 2^20 blocks  as defined in NIST SP 800-38E) */
   IPP_BADARG_RET(encBitsize > (1<<27), ippStsBadArgErr);

   /* test dataUnitBitsize and aesBlkNo */
   IPP_BADARG_RET(((dataUnitBitsize/IPP_AES_BLOCK_BITSIZE)<=aesBlkNo) || (0>aesBlkNo), ippStsBadArgErr);

   {
      IppStatus sts = ippStsNoErr;

      int keySize = keyBitsize/2/8;
      const Ipp8u* pConfKey = pKey;
      const Ipp8u* pTweakKey = pKey+keySize;

      do {
         int encBlocks = encBitsize/IPP_AES_BLOCK_BITSIZE;
         int encBlocklast = encBitsize%IPP_AES_BLOCK_BITSIZE;

         __ALIGN16 IppsAESSpec aesCtx;
         __ALIGN16 Ipp8u tweakCT[AES_BLK_SIZE];
         __ALIGN16 Ipp8u tmp[AES_BLKS_PER_BUFFER*AES_BLK_SIZE];

         sts = ippsAESInit(pTweakKey, keySize, &aesCtx, sizeof(aesCtx));
         if(ippStsNoErr!=sts) break;

         {
            RijnCipher encoder = RIJ_ENCODER(&aesCtx);
            #if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
            encoder(pTweakPT, tweakCT, RIJ_NR(&aesCtx), RIJ_EKEYS(&aesCtx), RijEncSbox/*NULL*/);
            #else
            encoder(pTweakPT, tweakCT, RIJ_NR(&aesCtx), RIJ_EKEYS(&aesCtx), NULL);
            #endif
         }

         sts = ippsAESInit(pConfKey, keySize, &aesCtx, sizeof(aesCtx));
         if(ippStsNoErr!=sts) break;

         for(; aesBlkNo>0; aesBlkNo--)
            gf_mul_by_primitive(tweakCT);

         if(encBlocklast) encBlocks--;

         /*
         // decrypt data
         */
         #if (_IPP>=_IPP_P8) || (_IPP32E>=_IPP32E_Y8)
         /* use Intel(R) AES New Instructions version if possible */
         if(AES_NI_ENABLED==RIJ_AESNI(&aesCtx)) {
            #if(_IPP32E>=_IPP32E_K1)
            if (IsFeatureEnabled(ippCPUID_AVX512VAES)) {
               cpAESDecryptXTS_VAES(pDst, pSrc, encBlocks, RIJ_DKEYS(&aesCtx), RIJ_NR(&aesCtx), tweakCT);
            }
            else
            #endif
            cpAESDecryptXTS_AES_NI(pDst, pSrc, encBlocks, RIJ_DKEYS(&aesCtx), RIJ_NR(&aesCtx), tweakCT);
            pSrc += encBlocks*AES_BLK_SIZE;
            pDst += encBlocks*AES_BLK_SIZE;
         }
         else
         #endif
         {
            for(; encBlocks>=AES_BLKS_PER_BUFFER && ippStsNoErr==sts; encBlocks-=AES_BLKS_PER_BUFFER) {
               /* compute whitening tweaks */
               cpXTSwhitening(tmp, AES_BLKS_PER_BUFFER, tweakCT);
               /* pre-whitening */
               cpXTSxor16(pDst, pSrc, tmp, AES_BLKS_PER_BUFFER);

               sts = ippsAESDecryptECB(pDst, pDst, AES_BLKS_PER_BUFFER*AES_BLK_SIZE, &aesCtx);

               /* post-whitening */
               cpXTSxor16(pDst, pDst, tmp, AES_BLKS_PER_BUFFER);

               pSrc += AES_BLKS_PER_BUFFER*AES_BLK_SIZE;
               pDst += AES_BLKS_PER_BUFFER*AES_BLK_SIZE;
            }
            if(ippStsNoErr!=sts) break;

            if(encBlocks) {
               cpXTSwhitening(tmp, encBlocks, tweakCT);

               /* pre-whitening */
               cpXTSxor16(pDst, pSrc, tmp, encBlocks);

               ippsAESDecryptECB(pDst, pDst, AES_BLK_SIZE*encBlocks, &aesCtx);

               /* post-whitening */
               cpXTSxor16(pDst, pDst, tmp, encBlocks);

               pSrc += AES_BLK_SIZE*encBlocks;
               pDst += AES_BLK_SIZE*encBlocks;
            }
         }

         /* "stealing" - encrypt last partial block if is */
         if(encBlocklast) {
            int partBlockSize = encBlocklast/BYTESIZE;

            __ALIGN16 Ipp8u cc[AES_BLK_SIZE];
            __ALIGN16 Ipp8u pp[AES_BLK_SIZE];
            CopyBlock16(tweakCT, cc);
            gf_mul_by_primitive(cc);
            cpAES_XTS_DecBlock(pp, pSrc, cc, &aesCtx);

            CopyBlock16(pp, cc);
            CopyBlock(pSrc+AES_BLK_SIZE, cc, partBlockSize);

            encBlocklast %= BYTESIZE;
            if(encBlocklast) {
               Ipp8u partBlockMask = (Ipp8u)((0xFF)<<((BYTESIZE -encBlocklast) %BYTESIZE));
               Ipp8u x = pSrc[AES_BLK_SIZE+partBlockSize];
               Ipp8u y = cc[partBlockSize];
               x = (x & partBlockMask) | (y & ~partBlockMask);
               cc[partBlockSize] = x;
               pp[partBlockSize] &= partBlockMask;
               partBlockSize++;
            }
            cpAES_XTS_DecBlock(pDst, cc, tweakCT, &aesCtx);

            CopyBlock(pp, pDst+AES_BLK_SIZE, partBlockSize);

            /* clear secret data */
            PurgeBlock(pp, sizeof(pp));
         }

      } while(0);

      return sts;
   }
}
