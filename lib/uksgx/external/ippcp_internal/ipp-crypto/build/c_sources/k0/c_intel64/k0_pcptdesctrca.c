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
//     Encrypt/Decrypt byte data stream according to TDES (CTR mode)
// 
//  Contents:
//     ippsTDESEncryptCTR()
//     ippsTDESDecryptCTR()
// 
// 
*/

#include "owndefs.h"

#include "owncp.h"
#include "pcpdes.h"
#include "pcptool.h"

static
IppStatus TDES_CTR(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                   const IppsDESSpec* pCtx1,
                   const IppsDESSpec* pCtx2,
                   const IppsDESSpec* pCtx3,
                   Ipp8u* pCtrValue, int ctrNumBitSize)
{
   Ipp64u counter;
   Ipp64u  output;

   /* test contexts */
   IPP_BAD_PTR3_RET(pCtx1, pCtx2, pCtx3);

   IPP_BADARG_RET(!VALID_DES_ID(pCtx1), ippStsContextMatchErr);
   IPP_BADARG_RET(!VALID_DES_ID(pCtx2), ippStsContextMatchErr);
   IPP_BADARG_RET(!VALID_DES_ID(pCtx3), ippStsContextMatchErr);
   /* test source, target and counter block pointers */
   IPP_BAD_PTR3_RET(pSrc, pDst, pCtrValue);
   /* test stream length */
   IPP_BADARG_RET((len<1), ippStsLengthErr);
   /* test counter block size */
   IPP_BADARG_RET(((MBS_DES*8)<ctrNumBitSize)||(ctrNumBitSize<1), ippStsCTRSizeErr);

   /* copy counter */
   CopyBlock8(pCtrValue, &counter);

   /*
   // encrypt block-by-block aligned streams
   */
   while(len >= MBS_DES) {
      /* encrypt counter block */
      output = Cipher_DES(counter, DES_EKEYS(pCtx1), DESspbox);
      output = Cipher_DES(output,  DES_DKEYS(pCtx2), DESspbox);
      output = Cipher_DES(output,  DES_EKEYS(pCtx3), DESspbox);
      /* compute ciphertext block */
      XorBlock8(pSrc, &output, pDst);
      /* encrement counter block */
      StdIncrement((Ipp8u*)&counter,MBS_DES*8, ctrNumBitSize);

      pSrc += MBS_DES;
      pDst += MBS_DES;
      len  -= MBS_DES;
   }
   /*
   // encrypt last data block
   */
   if(len) {
      /* encrypt counter block */
      output = Cipher_DES(counter, DES_EKEYS(pCtx1), DESspbox);
      output = Cipher_DES(output,  DES_DKEYS(pCtx2), DESspbox);
      output = Cipher_DES(output,  DES_EKEYS(pCtx3), DESspbox);
      /* compute ciphertext block */
      XorBlock(pSrc, &output, pDst,len);
      /* encrement counter block */
      StdIncrement((Ipp8u*)&counter,MBS_DES*8, ctrNumBitSize);
   }

   /* update counter */
   CopyBlock8(&counter, pCtrValue);

   return ippStsNoErr;
}

/*F*
//    Name: ippsTDESEncryptCTR
//
// Purpose: Encrypt byte data stream according to TDES in CTR mode.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pCtx1 == NULL
//                            pCtx2 == NULL
//                            pCtx3 == NULL
//                            pSrc  == NULL
//                            pDst  == NULL
//                            pCtrValue ==NULL
//    ippStsContextMatchErr   pCtx1->idCtx != idCtxDES
//                            pCtx2->idCtx != idCtxDES
//                            pCtx3->idCtx != idCtxDES
//    ippStsLengthErr         len <1
//    ippStsCTRSizeErr        64 < ctrNumBitSize < 1
//    ippStsNoErr             no errors
//
// Parameters:
//    pSrc           pointer to the source data stream
//    pDst           pointer to the target data stream
//    len            plaintext stream length (bytes)
//    pCtx1-pCtx3    DES contexts
//    pCtrValue      pointer to the counter block
//    ctrNumBitSize  counter block size (bits)
//
// Note:
//    counter will updated on return
//
*F*/

IPPFUN(IppStatus, ippsTDESEncryptCTR,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                      const IppsDESSpec* pCtx1,
                                      const IppsDESSpec* pCtx2,
                                      const IppsDESSpec* pCtx3,
                                      Ipp8u* pCtrValue, int ctrNumBitSize))
{
   return TDES_CTR(pSrc,pDst,len, pCtx1,pCtx2,pCtx3, pCtrValue,ctrNumBitSize);
}

/*F*
//    Name: ippsTDESDecryptCTR
//
// Purpose: Decrypt byte data stream according to TDES in CTR mode.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pCtx1 == NULL
//                            pCtx2 == NULL
//                            pCtx3 == NULL
//                            pSrc  == NULL
//                            pDst  == NULL
//                            pCtrValue ==NULL
//    ippStsContextMatchErr   pCtx1->idCtx != idCtxDES
//                            pCtx2->idCtx != idCtxDES
//                            pCtx3->idCtx != idCtxDES
//    ippStsLengthErr         len <1
//    ippStsCTRSizeErr        64 < ctrNumBitSize < 1
//    ippStsNoErr             no errors
//
// Parameters:
//    pSrc           pointer to the source data stream
//    pDst           pointer to the target data stream
//    len            plaintext stream length (bytes)
//    pCtx1-pCtx3    DES contexts
//    pCtrValue      pointer to the counter block
//    ctrNumBitSize  counter block size (bits)
//
// Note:
//    counter will updated on return
//
*F*/

IPPFUN(IppStatus, ippsTDESDecryptCTR,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                      const IppsDESSpec* pCtx1,
                                      const IppsDESSpec* pCtx2,
                                      const IppsDESSpec* pCtx3,
                                      Ipp8u* pCtrValue, int ctrNumBitSize))
{
   return TDES_CTR(pSrc,pDst,len, pCtx1,pCtx2,pCtx3, pCtrValue,ctrNumBitSize);
}
