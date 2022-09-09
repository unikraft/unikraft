/*******************************************************************************
* Copyright 2006-2021 Intel Corporation
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
//     Encrypt byte data stream according to TDES (OFB mode)
// 
//  Contents:
//     ippsTDESEncryptOCB(), ippsTDESDecryptOCB()
// 
// 
*/

#include "owndefs.h"

#include "owncp.h"
#include "pcpdes.h"
#include "pcptool.h"
static
void cpTDES_OFB8(const Ipp8u *pSrc, Ipp8u *pDst, int len, int ofbBlkSize,
                  const IppsDESSpec* pCtx1,
                  const IppsDESSpec* pCtx2,
                  const IppsDESSpec* pCtx3,
                        Ipp8u* pIV)
{
   Ipp64u inpBuffer;
   Ipp64u outBuffer;

   CopyBlock8(pIV, &inpBuffer);

   while(len>=ofbBlkSize) {
      /* block-by-block processing */
      outBuffer = Cipher_DES(inpBuffer, DES_EKEYS(pCtx1), DESspbox);
      outBuffer = Cipher_DES(outBuffer, DES_DKEYS(pCtx2), DESspbox);
      outBuffer = Cipher_DES(outBuffer, DES_EKEYS(pCtx3), DESspbox);

      /* store output */
      XorBlock(pSrc, &outBuffer, pDst, ofbBlkSize);

      /* shift inpBuffer for the next OFB operation */
      if(MBS_DES==ofbBlkSize)
         inpBuffer = outBuffer;
      else
         #if (IPP_ENDIAN == IPP_BIG_ENDIAN)
         inpBuffer = LSL64(inpBuffer, ofbBlkSize*8)
                    |LSR64(outBuffer, 64-ofbBlkSize*8);
         #else
         inpBuffer = LSR64(inpBuffer, ofbBlkSize*8)
                    |LSL64(outBuffer, 64-ofbBlkSize*8);
         #endif

      pSrc += ofbBlkSize;
      pDst += ofbBlkSize;
      len  -= ofbBlkSize;
   }

   /* update pIV */
   CopyBlock8(&inpBuffer, pIV);
}


/*F*
//    Name: ippsTDESEncryptOFB
//
// Purpose: Encrypts byte data stream according to TDES in OFB mode.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pCtx1== NULL
//                            pCtx2== NULL
//                            pCtx3== NULL
//                            pSrc == NULL
//                            pDst == NULL
//                            pIV  == NULL
//    ippStsContextMatchErr   pCtx->idCtx != idCtxRijndael
//    ippStsLengthErr         len <1
//    ippStsOFBSizeErr        (1>ofbBlkSize || ofbBlkSize>MBS_DES)
//    ippStsUnderRunErr       (len%ofbBlkSize)
//    ippStsNoErr             no errors
//
// Parameters:
//    pSrc        pointer to the source data stream
//    pDst        pointer to the target data stream
//    len         plaintext stream length (bytes)
//    ofbBlkSize  OFB block size (bytes)
//    pCtx1,..    DES contexts
//    pIV         pointer to the initialization vector
//
*F*/
IPPFUN(IppStatus, ippsTDESEncryptOFB,(const Ipp8u* pSrc, Ipp8u* pDst, int len, int ofbBlkSize,
                                      const IppsDESSpec* pCtx1,
                                      const IppsDESSpec* pCtx2,
                                      const IppsDESSpec* pCtx3,
                                            Ipp8u* pIV))
{
   /* test contexts */
   IPP_BAD_PTR3_RET(pCtx1, pCtx2, pCtx3);

   /* test context validity */
   IPP_BADARG_RET(!VALID_DES_ID(pCtx1), ippStsContextMatchErr);
   IPP_BADARG_RET(!VALID_DES_ID(pCtx2), ippStsContextMatchErr);
   IPP_BADARG_RET(!VALID_DES_ID(pCtx3), ippStsContextMatchErr);

   /* test source and destination pointers */
   IPP_BAD_PTR3_RET(pSrc, pDst, pIV);
   /* test stream length */
   IPP_BADARG_RET((len<1), ippStsLengthErr);
   /* test OFB value */
   IPP_BADARG_RET(((1>ofbBlkSize) || (MBS_DES<ofbBlkSize)), ippStsOFBSizeErr);
   /* test stream integrity */
   IPP_BADARG_RET((len % ofbBlkSize), ippStsUnderRunErr);

   cpTDES_OFB8(pSrc, pDst, len, ofbBlkSize, pCtx1,pCtx2,pCtx3, pIV);
   return ippStsNoErr;
}

/*F*
//    Name: ippsTDESDecryptOFB
//
// Purpose: Decrypts byte data stream according to TDES in OFB mode.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pCtx1== NULL
//                            pCtx2== NULL
//                            pCtx3== NULL
//                            pSrc == NULL
//                            pDst == NULL
//                            pIV  == NULL
//    ippStsContextMatchErr   pCtx->idCtx != idCtxRijndael
//    ippStsLengthErr         len <1
//    ippStsOFBSizeErr        (1>ofbBlkSize || ofbBlkSize>MBS_DES)
//    ippStsUnderRunErr       (len%ofbBlkSize)
//    ippStsNoErr             no errors
//
// Parameters:
//    pSrc        pointer to the source data stream
//    pDst        pointer to the target data stream
//    len         plaintext stream length (bytes)
//    ofbBlkSize  OFB block size (bytes)
//    pCtx1,..    DES contexts
//    pIV         pointer to the initialization vector
//
*F*/

IPPFUN(IppStatus, ippsTDESDecryptOFB,(const Ipp8u* pSrc, Ipp8u* pDst, int len, int ofbBlkSize,
                                      const IppsDESSpec* pCtx1,
                                      const IppsDESSpec* pCtx2,
                                      const IppsDESSpec* pCtx3,
                                            Ipp8u* pIV))
{
   /* test contexts */
   IPP_BAD_PTR3_RET(pCtx1, pCtx2, pCtx3);

   /* test context validity */
   IPP_BADARG_RET(!VALID_DES_ID(pCtx1), ippStsContextMatchErr);
   IPP_BADARG_RET(!VALID_DES_ID(pCtx2), ippStsContextMatchErr);
   IPP_BADARG_RET(!VALID_DES_ID(pCtx3), ippStsContextMatchErr);

   /* test source and destination pointers */
   IPP_BAD_PTR3_RET(pSrc, pDst, pIV);
   /* test stream length */
   IPP_BADARG_RET((len<1), ippStsLengthErr);
   /* test OFB value */
   IPP_BADARG_RET(((1>ofbBlkSize) || (MBS_DES<ofbBlkSize)), ippStsOFBSizeErr);
   /* test stream integrity */
   IPP_BADARG_RET((len % ofbBlkSize), ippStsUnderRunErr);

   cpTDES_OFB8(pSrc, pDst, len, ofbBlkSize, pCtx1,pCtx2,pCtx3, pIV);
   return ippStsNoErr;
}
