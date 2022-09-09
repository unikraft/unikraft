/*******************************************************************************
* Copyright 2002-2021 Intel Corporation
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
//     Encrypt byte data stream according to TDES (CFB mode)
// 
//  Contents:
//     ippsTDESEncryptCFB()
// 
// 
*/

#include "owndefs.h"

#include "owncp.h"
#include "pcpdes.h"
#include "pcptool.h"


/*F*
//    Name: ippsTDESEncryptCFB
//
// Purpose: Encrypt byte data block stream according to TDES in CFB mode.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pCtx1 == NULL
//                            pCtx2 == NULL
//                            pCtx3 == NULL
//                            pSrc == NULL
//                            pDst == NULL
//    ippStsContextMatchErr   pCtx1->idCtx != idCtxDES
//                            pCtx2->idCtx != idCtxDES
//                            pCtx3->idCtx != idCtxDES
//    ippStsLengthErr         len <1
//    ippStsCFBSizeErr        (1>cfbBlkSize || cfbBlkSize>MBS_DES)
//    ippStsUnderRunErr       (len%cfbBlkSize)
//    ippStsNoErr             no errors
//
// Parameters:
//    pSrc        pointer to the source byte data stream
//    pDst        pointer to the target byte data stream
//    len         plaintext stream length (bytes)
//    cfbBlkSize  CFB block size (bytes)
//    pCtx1-3     pointers to the DES context
//    pIV         pointer to the init vector
//    padding     the padding scheme indicator
//
*F*/
IPPFUN(IppStatus, ippsTDESEncryptCFB,(const Ipp8u* pSrc, Ipp8u* pDst, int len, int cfbBlkSize,
                                      const IppsDESSpec* pCtx1, const IppsDESSpec* pCtx2, const IppsDESSpec* pCtx3,
                                      const Ipp8u* pIV,
                                      IppsPadding padding))
{
   Ipp64u inpBuffer;
   Ipp64u outBuffer;

   /* test contexts */
   IPP_BAD_PTR3_RET(pCtx1, pCtx2, pCtx3);

   IPP_BADARG_RET(!VALID_DES_ID(pCtx1), ippStsContextMatchErr);
   IPP_BADARG_RET(!VALID_DES_ID(pCtx2), ippStsContextMatchErr);
   IPP_BADARG_RET(!VALID_DES_ID(pCtx3), ippStsContextMatchErr);
   /* test source and destination pointers */
   IPP_BAD_PTR3_RET(pSrc, pDst, pIV);
   /* test stream length */
   IPP_BADARG_RET((len<1), ippStsLengthErr);
   /* test CFB value */
   IPP_BADARG_RET(((1>cfbBlkSize) || (MBS_DES<cfbBlkSize)), ippStsCFBSizeErr);

   /* force ippPaddingNONE padding */
   if(ippPaddingNONE!=padding)
      padding = ippPaddingNONE;
   /* test stream integrity */
   //IPP_BADARG_RET(((len%cfbBlkSize) && (ippPaddingNONE==padding)), ippStsUnderRunErr);
   IPP_BADARG_RET((len%cfbBlkSize), ippStsUnderRunErr);

   /* read IV */
   CopyBlock8(pIV, &inpBuffer);

   /* encrypt data block-by-block of cfbLen each */
   while(len>=cfbBlkSize) {
      int n;

      /* encryption */
      outBuffer = Cipher_DES(inpBuffer, DES_EKEYS(pCtx1), DESspbox);
      outBuffer = Cipher_DES(outBuffer, DES_DKEYS(pCtx2), DESspbox);
      outBuffer = Cipher_DES(outBuffer, DES_EKEYS(pCtx3), DESspbox);

      /* store output and put feedback into the buffer */
      for(n=0; n<cfbBlkSize; n++) {
         pDst[n] = (Ipp8u)( ((Ipp8u*)&outBuffer)[n] ^ pSrc[n] );
         ((Ipp8u*)&outBuffer)[n] = pDst[n];
      }

      /* shift buffer for the next CFB operation */
      if(MBS_DES==cfbBlkSize)
         inpBuffer = outBuffer;
      else
         #if (IPP_ENDIAN == IPP_BIG_ENDIAN)
         inpBuffer = LSL64(inpBuffer, cfbBlkSize*8)
                    |LSR64(outBuffer, 64-cfbBlkSize*8);
         #else
         inpBuffer = LSR64(inpBuffer, cfbBlkSize*8)
                    |LSL64(outBuffer, 64-cfbBlkSize*8);
         #endif

      pSrc += cfbBlkSize;
      pDst += cfbBlkSize;
      len  -= cfbBlkSize;
   }

   return ippStsNoErr;
}
