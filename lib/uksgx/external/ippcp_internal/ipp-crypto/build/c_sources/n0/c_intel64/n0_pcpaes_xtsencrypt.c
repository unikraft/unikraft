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
//        ippsAES_XTSEncrypt()
//
*/

#include "owncp.h"
#include "pcpaesmxts.h"
#include "pcptool.h"
#include "pcpaesmxtsstuff.h"


#if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
#  include "pcprijtables.h"
#endif

static void cpAES_XTS_EncBlock(Ipp8u* ctxt, const Ipp8u* ptxt, const Ipp8u* tweak, const IppsAESSpec* pEncCtx)
{
   /* pre-whitening */
   XorBlock16(ptxt, tweak, ctxt);
   /* encryption */
   ippsAESEncryptECB(ctxt, ctxt, AES_BLK_SIZE, pEncCtx);
   /* post-whitening */
   XorBlock16(ctxt, tweak, ctxt);
}

/*F*
//    Name: ippsAES_XTSEncrypt
//
// Purpose: AES-XTS encryption.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pSrc == NULL
//                            pDst == NULL
//                            pTweak ==NULL
//                            pCtx == NULL
//    ippStsLengthErr         bitLen <128
//    ippStsContextMatchErr   !VALID_AES_XTS_ID(pCtx)
//    ippStsBadArgErr         !IsLegalGeometry(startCipherBlkNo,
//                               bitLen, pCtx->duBitsize)
//    ippStsNoErr             no errors
//
// Parameters:
//    pSrc              points input buffer
//    pDst              points output buffer
//    bitLen            length of the input buffer in bits
//    startCipherBlkNo  number of the first block for data unit
//    pTweak            points tweak value
//    pCtx              points AES_XTS context
//
*F*/

IPPFUN(IppStatus, ippsAES_XTSEncrypt,(const Ipp8u* pSrc, Ipp8u* pDst, int bitLen,
                                      const IppsAES_XTSSpec* pCtx,
                                      const Ipp8u* pTweak,
                                      int startCipherBlkNo))
{
   /* test pointers */
   IPP_BAD_PTR1_RET(pCtx);
   /* test the context ID */
   IPP_BADARG_RET(!VALID_AES_XTS_ID(pCtx), ippStsContextMatchErr);

   /* test data pointers */
   IPP_BAD_PTR3_RET(pSrc, pDst, pTweak);

   /* test startCipherBlkNo and bitLen */
   IPP_BADARG_RET(bitLen < IPP_AES_BLOCK_BITSIZE, ippStsLengthErr);
   IPP_BADARG_RET(!IsLegalGeometry(startCipherBlkNo, bitLen, pCtx->duBitsize), ippStsBadArgErr);

   {
      __ALIGN16 Ipp8u tweakCT[AES_BLK_SIZE];

      { /* encrypt tweak */
         const IppsAESSpec* ptwkAES = &pCtx->tweakAES;

         RijnCipher encoder = RIJ_ENCODER(ptwkAES);
         #if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
         encoder(pTweak, tweakCT, RIJ_NR(ptwkAES), RIJ_EKEYS(ptwkAES), RijEncSbox/*NULL*/);
         #else
         encoder(pTweak, tweakCT, RIJ_NR(ptwkAES), RIJ_EKEYS(ptwkAES), NULL);
         #endif

         /* update tweakCT */
         for(; startCipherBlkNo>0; startCipherBlkNo--)
            gf_mul_by_primitive(tweakCT);
      }

      /* XTS encryption */
      {
         const IppsAESSpec* pdatAES = &pCtx->datumAES;

         int encBlocks = bitLen/IPP_AES_BLOCK_BITSIZE;
         int encBlocklast = bitLen%IPP_AES_BLOCK_BITSIZE;
         if(encBlocklast) encBlocks--;

         /* encrypt data blocks */
         if( encBlocks>0) {
            #if (_IPP>=_IPP_P8) || (_IPP32E>=_IPP32E_Y8)
            /* use Intel(R) AES New Instructions version if possible */
            if(AES_NI_ENABLED==RIJ_AESNI(pdatAES)) {
               #if(_IPP32E>=_IPP32E_K1)
               if (IsFeatureEnabled(ippCPUID_AVX512VAES)) {
                  cpAESEncryptXTS_VAES(pDst, pSrc, encBlocks, RIJ_EKEYS(pdatAES), RIJ_NR(pdatAES), tweakCT);
               }
               else
               #endif
               cpAESEncryptXTS_AES_NI(pDst, pSrc, encBlocks, RIJ_EKEYS(pdatAES), RIJ_NR(pdatAES), tweakCT);
               pSrc += encBlocks*AES_BLK_SIZE;
               pDst += encBlocks*AES_BLK_SIZE;
            }
            else
            #endif
            {
               for(; encBlocks>0; encBlocks--) {
                  cpAES_XTS_EncBlock(pDst, pSrc, tweakCT, pdatAES);
                  gf_mul_by_primitive(tweakCT);
                  pSrc += AES_BLK_SIZE;
                  pDst += AES_BLK_SIZE;
               }
            }
         }

         /* "stealing" - encrypt last partial block if is */
         if(encBlocklast) {
            int partBlockSize = encBlocklast/BYTESIZE;

            __ALIGN16 Ipp8u cc[AES_BLK_SIZE];
            __ALIGN16 Ipp8u pp[AES_BLK_SIZE];
            cpAES_XTS_EncBlock(cc, pSrc, tweakCT, pdatAES);
            gf_mul_by_primitive(tweakCT);

            CopyBlock16(cc, pp);
            CopyBlock(pSrc+AES_BLK_SIZE, pp, partBlockSize);

            encBlocklast %= BYTESIZE;
            if(encBlocklast) {
               Ipp8u partBlockMask = (Ipp8u)((0xFF)<<((BYTESIZE -encBlocklast) %BYTESIZE));
               Ipp8u x = pSrc[AES_BLK_SIZE+partBlockSize];
               Ipp8u y = cc[partBlockSize];
               x = (x & partBlockMask) | (y & ~partBlockMask);
               pp[partBlockSize] = x;
               cc[partBlockSize] &= partBlockMask;
               partBlockSize++;
            }
            cpAES_XTS_EncBlock(pDst, pp, tweakCT, pdatAES);

            CopyBlock(cc, pDst+AES_BLK_SIZE, partBlockSize);
         }
         return ippStsNoErr;
      }
   }
}

