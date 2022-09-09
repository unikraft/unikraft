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
//     AES encryption/decryption (CFB mode)
// 
//  Contents:
//        ippsAESEncryptCFB()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpaesm.h"
#include "pcptool.h"

#if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
#  include "pcprijtables.h"
#endif


/*F*
//    Name: ippsAESEncryptCFB
//
// Purpose: AES-CFB encryption.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pCtx == NULL
//                            pSrc == NULL
//                            pDst == NULL
//                            pIV  == NULL
//    ippStsContextMatchErr   !VALID_AES_ID()
//    ippStsLengthErr         len <1
//    ippStsCFBSizeErr        (1>cfbBlkSize || cfbBlkSize>MBS_RIJ128)
//    ippStsUnderRunErr       0!=(dataLen%cfbBlkSize)
//    ippStsNoErr             no errors
//
// Parameters:
//    pSrc        pointer to the source data buffer
//    pDst        pointer to the target data buffer
//    len         input buffer length (in bytes)
//    cfbBlkSize  CFB block size (in bytes)
//    pCtx        pointer to the AES context
//    pIV         pointer to the initialization vector
//
*F*/
IPPFUN(IppStatus, ippsAESEncryptCFB,(const Ipp8u* pSrc, Ipp8u* pDst, int len, int cfbBlkSize,
                                     const IppsAESSpec* pCtx,
                                     const Ipp8u* pIV))
{
   /* test context */
   IPP_BAD_PTR1_RET(pCtx);
   /* test the context ID */
   IPP_BADARG_RET(!VALID_AES_ID(pCtx), ippStsContextMatchErr);

   /* test source, target buffers and initialization pointers */
   IPP_BAD_PTR3_RET(pSrc, pIV, pDst);
   /* test stream length */
   IPP_BADARG_RET((len<1), ippStsLengthErr);
   /* test CFB value */
   IPP_BADARG_RET(((1>cfbBlkSize) || (MBS_RIJ128<cfbBlkSize)), ippStsCFBSizeErr);

   /* test stream integrity */
   IPP_BADARG_RET((len%cfbBlkSize), ippStsUnderRunErr);


#if (_IPP>=_IPP_P8) || (_IPP32E>=_IPP32E_Y8)
   /* use pipelined version is possible */
   if(AES_NI_ENABLED==RIJ_AESNI(pCtx)) {
      if(cfbBlkSize==MBS_RIJ128)
         EncryptCFB128_RIJ128_AES_NI(pSrc, pDst, RIJ_NR(pCtx), RIJ_EKEYS(pCtx), len, pIV);
      else if(0==(cfbBlkSize&3))
         EncryptCFB32_RIJ128_AES_NI(pSrc, pDst, RIJ_NR(pCtx), RIJ_EKEYS(pCtx), len, cfbBlkSize, pIV);
      else
         EncryptCFB_RIJ128_AES_NI(pSrc, pDst, RIJ_NR(pCtx), RIJ_EKEYS(pCtx), len, cfbBlkSize, pIV);
      return ippStsNoErr;
   }
   else
#endif

   {
      Ipp32u tmpInp[2*NB(128)];
      Ipp32u tmpOut[  NB(128)];

      /* setup encoder method */
      RijnCipher encoder = RIJ_ENCODER(pCtx);

      /* read IV */
      CopyBlock16(pIV, tmpInp);

      /* encrypt data block-by-block of cfbLen each */
      while(len>=cfbBlkSize) {
         int n;

         /* encryption */
         //encoder(tmpInp, tmpOut, RIJ_NR(pCtx), RIJ_EKEYS(pCtx), (const Ipp32u (*)[256])RIJ_ENC_SBOX(pCtx));
         #if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
         encoder((Ipp8u*)tmpInp, (Ipp8u*)tmpOut, RIJ_NR(pCtx), RIJ_EKEYS(pCtx), RijEncSbox/*NULL*/);
         #else
         encoder((Ipp8u*)tmpInp, (Ipp8u*)tmpOut, RIJ_NR(pCtx), RIJ_EKEYS(pCtx), NULL);
         #endif

         /* store output and put feedback into the input buffer (tmpInp) */
         if( cfbBlkSize==MBS_RIJ128 && pSrc!=pDst) {
            tmpInp[0] = ((Ipp32u*)pDst)[0] = tmpOut[0]^((Ipp32u*)pSrc)[0];
            tmpInp[1] = ((Ipp32u*)pDst)[1] = tmpOut[1]^((Ipp32u*)pSrc)[1];
            tmpInp[2] = ((Ipp32u*)pDst)[2] = tmpOut[2]^((Ipp32u*)pSrc)[2];
            tmpInp[3] = ((Ipp32u*)pDst)[3] = tmpOut[3]^((Ipp32u*)pSrc)[3];
         }
         else {
            for(n=0; n<cfbBlkSize; n++) {
               pDst[n] = (Ipp8u)( ((Ipp8u*)tmpOut)[n] ^ pSrc[n] );
               ((Ipp8u*)tmpInp)[MBS_RIJ128+n] = pDst[n];
            }

            /* shift input buffer (tmpInp) for the next CFB operation */
            CopyBlock16((Ipp8u*)tmpInp+cfbBlkSize, tmpInp);
         }

         pSrc += cfbBlkSize;
         pDst += cfbBlkSize;
         len -= cfbBlkSize;
      }

      return ippStsNoErr;
   }
}
