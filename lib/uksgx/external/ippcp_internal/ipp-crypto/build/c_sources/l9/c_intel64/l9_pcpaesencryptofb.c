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
//     AES encryption (OFB mode)
// 
//  Contents:
//        ippsAESEncryptOFB()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpaesm.h"
#include "pcptool.h"
#include "pcpaes_ofb.h"

/*F*
//    Name: ippsAESEncryptOFB
//
// Purpose: Encrypts byte data stream according to Rijndael in OFB mode.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pCtx == NULL
//                            pSrc == NULL
//                            pDst == NULL
//                            pIV  == NULL
//    ippStsContextMatchErr   !VALID_AES_ID()
//    ippStsLengthErr         len <1
//    ippStsOFBSizeErr        (1>ofbBlkSize || ofbBlkSize>MBS_RIJ128)
//    ippStsUnderRunErr       (len%ofbBlkSize)
//    ippStsNoErr             no errors
//
// Parameters:
//    pSrc        pointer to the source data buffer
//    pDst        pointer to the target data buffer
//    len         input buffer length (in bytes)
//    ofbBlkSize  OFB block size (in bytes)
//    pCtx        pointer to the AES context
//    pIV         pointer to the initialization vector
*F*/
IPPFUN(IppStatus, ippsAESEncryptOFB,(const Ipp8u* pSrc, Ipp8u* pDst, int len, int ofbBlkSize,
                                     const IppsAESSpec* pCtx,
                                     Ipp8u* pIV))
{
   /* test context */
   IPP_BAD_PTR1_RET(pCtx);
   /* test the context ID */
   IPP_BADARG_RET(!VALID_AES_ID(pCtx), ippStsContextMatchErr);

   /* test source, target buffers and initialization pointers */
   IPP_BAD_PTR3_RET(pSrc, pIV, pDst);
   /* test stream length */
   IPP_BADARG_RET((len<1), ippStsLengthErr);
   /* test OFB value */
   IPP_BADARG_RET(((1>ofbBlkSize) || (MBS_RIJ128<ofbBlkSize)), ippStsOFBSizeErr);
   /* test stream integrity */
   IPP_BADARG_RET((len%ofbBlkSize), ippStsUnderRunErr);

#if (_IPP>=_IPP_P8) || (_IPP32E>=_IPP32E_Y8)
   if(AES_NI_ENABLED==RIJ_AESNI(pCtx)) {
      if(ofbBlkSize==MBS_RIJ128)
         EncryptOFB128_RIJ128_AES_NI(pSrc, pDst, RIJ_NR(pCtx), RIJ_EKEYS(pCtx), len, pIV);
      else
         EncryptOFB_RIJ128_AES_NI(pSrc, pDst, RIJ_NR(pCtx), RIJ_EKEYS(pCtx), len, ofbBlkSize, pIV);
      return ippStsNoErr;
   }
   else
#endif
   {
      cpProcessAES_ofb8(pSrc, pDst, len, ofbBlkSize, pCtx, pIV);
      return ippStsNoErr;
   }
}
