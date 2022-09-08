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
//     AES encryption/decryption (CTR mode)
//
//  Contents:
//     cpProcessAES_ctr()
//     cpProcessAES_ctr128()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpaesm.h"
#include "pcptool.h"

#if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
#  include "pcprijtables.h"
#endif

/*
// AES-CRT processing.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pCtx == NULL
//                            pSrc == NULL
//                            pDst == NULL
//                            pCtrValue ==NULL
//    ippStsContextMatchErr   !VALID_AES_ID()
//    ippStsLengthErr         len <1
//    ippStsCTRSizeErr        128 < ctrNumBitSize < 1
//    ippStsCTRSizeErr        data blocks number > 2^ctrNumBitSize
//    ippStsNoErr             no errors
//
// Parameters:
//    pSrc           pointer to the source data buffer
//    pDst           pointer to the target data buffer
//    dataLen        input/output buffer length (in bytes)
//    pCtx           pointer to rge AES context
//    pCtrValue      pointer to the counter block
//    ctrNumBitSize  counter block size (bits)
//
// Note:
//    counter will updated on return
//
*/
__INLINE void MaskCounter128(Ipp8u* pMaskIV, int ctrBtSize)
{
   /* construct ctr mask */
   int maskPosition = (MBS_RIJ128*8-ctrBtSize)/8;
   Ipp8u maskValue = (Ipp8u)(0xFF >> (MBS_RIJ128*8-ctrBtSize)%8 );

   //Ipp8u maskIV[MBS_RIJ128];
   int n;
   for(n=0; n<MBS_RIJ128; n++) {
      int d = n - maskPosition;
      Ipp8u storedMaskValue = maskValue & ~cpIsMsb_ct((BNU_CHUNK_T)d);
      pMaskIV[n] = storedMaskValue;
      maskValue |= ~cpIsMsb_ct((BNU_CHUNK_T)d);
   }
}

static
IppStatus cpProcessAES_ctr(const Ipp8u* pSrc, Ipp8u* pDst, int dataLen,
                           const IppsAESSpec* pCtx,
                           Ipp8u* pCtrValue, int ctrNumBitSize)
{
   /* test context */
   IPP_BAD_PTR1_RET(pCtx);
   /* test the context ID */
   IPP_BADARG_RET(!VALID_AES_ID(pCtx), ippStsContextMatchErr);

   /* test source, target and counter block pointers */
   IPP_BAD_PTR3_RET(pSrc, pDst, pCtrValue);
   /* test stream length */
   IPP_BADARG_RET((dataLen<1), ippStsLengthErr);

   /* test counter block size */
   IPP_BADARG_RET(((MBS_RIJ128*8)<ctrNumBitSize)||(ctrNumBitSize<1), ippStsCTRSizeErr);

   /* test counter overflow */
   if(ctrNumBitSize < (8 * sizeof(int) - 5))
   {
      /*
      // dataLen is int, and it is always positive   
      // data blocks number compute from dataLen     
      // by dividing it to MBS_RIJ128 = 16           
      // and additing 1 if dataLen % 16 != 0         
      // so if ctrNumBitSize >= 8 * sizeof(int) - 5                      
      // function can process data with any possible 
      // passed dataLen without counter overflow     
      */
      
      int dataBlocksNum = dataLen >> 4;
      if(dataLen & 15){
         dataBlocksNum++;
      }

      IPP_BADARG_RET(dataBlocksNum > (1 << ctrNumBitSize), ippStsCTRSizeErr);
   }

   #if (_IPP>=_IPP_P8) || (_IPP32E>=_IPP32E_Y8)
   /* use pipelined version if possible */
   if(AES_NI_ENABLED==RIJ_AESNI(pCtx)) {
      /* construct ctr mask */
      Ipp8u maskIV[MBS_RIJ128];
      MaskCounter128(maskIV, ctrNumBitSize); /* const-exe-time version */

#if(_IPP32E>=_IPP32E_K1)
      if (IsFeatureEnabled(ippCPUID_AVX512VAES)) {
         EncryptCTR_RIJ128pipe_VAES_NI(pSrc, pDst, RIJ_NR(pCtx), RIJ_EKEYS(pCtx), dataLen, pCtrValue, maskIV);
      }
      else
#endif
      {
         EncryptCTR_RIJ128pipe_AES_NI(pSrc, pDst, RIJ_NR(pCtx), RIJ_EKEYS(pCtx), dataLen, pCtrValue, maskIV);
      }
      return ippStsNoErr;
   }
   else
   #endif
   {
      Ipp32u counter[NB(128)];
      Ipp32u  output[NB(128)];

      /* setup encoder method */
      RijnCipher encoder = RIJ_ENCODER(pCtx);

      /* copy counter */
      CopyBlock16(pCtrValue, counter);

      /*
      // encrypt block-by-block aligned streams
      */
      while(dataLen>= MBS_RIJ128) {
         /* encrypt counter block */
         //encoder(counter, output, RIJ_NR(pCtx), RIJ_EKEYS(pCtx), (const Ipp32u (*)[256])RIJ_ENC_SBOX(pCtx));
         #if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
         encoder((Ipp8u*)counter, (Ipp8u*)output, RIJ_NR(pCtx), RIJ_EKEYS(pCtx), RijEncSbox/*NULL*/);
         #else
         encoder((Ipp8u*)counter, (Ipp8u*)output, RIJ_NR(pCtx), RIJ_EKEYS(pCtx), NULL);
         #endif

         /* compute ciphertext block */
         if( !(IPP_UINT_PTR(pSrc) & 0x3) && !(IPP_UINT_PTR(pDst) & 0x3)) {
            ((Ipp32u*)pDst)[0] = output[0]^((Ipp32u*)pSrc)[0];
            ((Ipp32u*)pDst)[1] = output[1]^((Ipp32u*)pSrc)[1];
            ((Ipp32u*)pDst)[2] = output[2]^((Ipp32u*)pSrc)[2];
            ((Ipp32u*)pDst)[3] = output[3]^((Ipp32u*)pSrc)[3];
         }
         else
            XorBlock16(pSrc, output, pDst);
         /* encrement counter block */
         StdIncrement((Ipp8u*)counter,MBS_RIJ128*8, ctrNumBitSize);

         pSrc += MBS_RIJ128;
         pDst += MBS_RIJ128;
         dataLen -= MBS_RIJ128;
      }
      /*
      // encrypt last data block
      */
      if(dataLen) {
         /* encrypt counter block */
         //encoder(counter, output, RIJ_NR(pCtx), RIJ_EKEYS(pCtx), (const Ipp32u (*)[256])RIJ_ENC_SBOX(pCtx));
         #if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
         encoder((Ipp8u*)counter, (Ipp8u*)output, RIJ_NR(pCtx), RIJ_EKEYS(pCtx), RijEncSbox/*NULL*/);
         #else
         encoder((Ipp8u*)counter, (Ipp8u*)output, RIJ_NR(pCtx), RIJ_EKEYS(pCtx), NULL);
         #endif

         /* compute ciphertext block */
         XorBlock(pSrc, output, pDst,dataLen);
         /* encrement counter block */
         StdIncrement((Ipp8u*)counter,MBS_RIJ128*8, ctrNumBitSize);
      }

      /* update counter */
      CopyBlock16(counter, pCtrValue);

      return ippStsNoErr;
   }
}

#if (_IPP32E>=_IPP32E_Y8)

/*
// special version: 128-bit counter
*/
static
IppStatus cpProcessAES_ctr128(const Ipp8u* pSrc, Ipp8u* pDst, int dataLen, const IppsAESSpec* pCtx, Ipp8u* pCtrValue)
{
   /* test context */
   IPP_BAD_PTR1_RET(pCtx);
   /* test the context ID */
   IPP_BADARG_RET(!VALID_AES_ID(pCtx), ippStsContextMatchErr);

   /* test source, target and counter block pointers */
   IPP_BAD_PTR3_RET(pSrc, pDst, pCtrValue);
   /* test stream length */
   IPP_BADARG_RET((dataLen<1), ippStsLengthErr);

   {
      while(dataLen>=MBS_RIJ128) {
         Ipp32u blocks = (Ipp32u)(dataLen>>4); /* number of blocks per loop processing */

         /* low LE 32 bit of counter */
         Ipp32u ctr32 = ((Ipp32u*)(pCtrValue))[3];
         ctr32 = ENDIANNESS32(ctr32);

         /* compute number of locks being processed without ctr32 overflow */
         ctr32 += blocks;
         if(ctr32 < blocks)
            blocks -= ctr32;

#if(_IPP32E>=_IPP32E_K1)
         if (IsFeatureEnabled(ippCPUID_AVX512VAES)) {
            EncryptStreamCTR32_VAES_NI(pSrc, pDst, RIJ_NR(pCtx), RIJ_EKEYS(pCtx), (Ipp32s)blocks*MBS_RIJ128, pCtrValue);
         }
         else
#endif
         EncryptStreamCTR32_AES_NI(pSrc, pDst, RIJ_NR(pCtx), RIJ_EKEYS(pCtx), (Ipp32s)blocks*MBS_RIJ128, pCtrValue);

         pSrc += blocks*MBS_RIJ128;
         pDst += blocks*MBS_RIJ128;
         dataLen -= blocks*MBS_RIJ128;
      }

      if(dataLen) {
         EncryptStreamCTR32_AES_NI(pSrc, pDst, RIJ_NR(pCtx), RIJ_EKEYS(pCtx), dataLen, pCtrValue);
      }

      return ippStsNoErr;
   }
}

#endif /* #if (_IPP32E>=_IPP32E_Y8) */
