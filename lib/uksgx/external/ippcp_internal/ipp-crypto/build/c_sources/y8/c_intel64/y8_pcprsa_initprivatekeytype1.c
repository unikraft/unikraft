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
//     RSA Functions
// 
//  Contents:
//        ippsRSA_InitPrivateKeyType1()
//
*/

#include "owncp.h"
#include "pcpbn.h"
#include "pcpngrsa.h"

#include "pcprsa_sizeof_privkey1.h"

/*F*
// Name: ippsRSA_InitPrivateKeyType1
//
// Purpose: Init RSA private key context
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pKey
//
//    ippStsNotSupportedModeErr  MIN_RSA_SIZE > rsaModulusBitSize
//                               MAX_RSA_SIZE < rsaModulusBitSize
//
//    ippStsBadArgErr            0 >= privateExpBitSize
//                               privateExpBitSize > rsaModulusBitSize
//
//    ippStsMemAllocErr          keyCtxSize is not enough for operation
//
//    ippStsNoErr                no error
//
// Parameters:
//    rsaModulusBitSize    bitsize of RSA modulus (bitsize of N)
//    privateExpBitSize    bitsize of private exponent (bitsize of D)
//    pKey                 pointer to the key context
//    keyCtxSize           size of memmory accosizted with key comtext
*F*/

IPPFUN(IppStatus, ippsRSA_InitPrivateKeyType1,(int rsaModulusBitSize, int privateExpBitSize,
                                               IppsRSAPrivateKeyState* pKey, int keyCtxSize))
{
   IPP_BAD_PTR1_RET(pKey);

   IPP_BADARG_RET((MIN_RSA_SIZE>rsaModulusBitSize) || (rsaModulusBitSize>MAX_RSA_SIZE), ippStsNotSupportedModeErr);
   IPP_BADARG_RET(!((0<privateExpBitSize) && (privateExpBitSize<=rsaModulusBitSize)), ippStsBadArgErr);

   /* test available size of context buffer */
   //IPP_BADARG_RET(keyCtxSize<cpSizeof_RSA_privateKey1(rsaModulusBitSize,privateExpBitSize), ippStsMemAllocErr);
   IPP_BADARG_RET(keyCtxSize<cpSizeof_RSA_privateKey1(rsaModulusBitSize,rsaModulusBitSize), ippStsMemAllocErr);

   RSA_PRV_KEY1_SET_ID(pKey);
   RSA_PRV_KEY_MAXSIZE_N(pKey) = rsaModulusBitSize;
   RSA_PRV_KEY_MAXSIZE_D(pKey) = privateExpBitSize;
   RSA_PRV_KEY_BITSIZE_N(pKey) = 0;
   RSA_PRV_KEY_BITSIZE_D(pKey) = 0;
   RSA_PRV_KEY_BITSIZE_P(pKey) = 0;
   RSA_PRV_KEY_BITSIZE_Q(pKey) = 0;

   RSA_PRV_KEY_DP(pKey) = NULL;
   RSA_PRV_KEY_DQ(pKey) = NULL;
   RSA_PRV_KEY_INVQ(pKey) = NULL;
   RSA_PRV_KEY_PMONT(pKey) = NULL;
   RSA_PRV_KEY_QMONT(pKey) = NULL;

   {
      Ipp8u* ptr = (Ipp8u*)pKey;

      //int prvExpLen = BITS_BNU_CHUNK(privateExpBitSize);
      int prvExpLen = BITS_BNU_CHUNK(rsaModulusBitSize);
      int modulusLen32 = BITS2WORD32_SIZE(rsaModulusBitSize);
      int montNsize;
      rsaMontExpGetSize(modulusLen32, &montNsize);

      /* allocate internal contexts */
      ptr += sizeof(IppsRSAPrivateKeyState);

      RSA_PRV_KEY_D(pKey) = (BNU_CHUNK_T*)( IPP_ALIGNED_PTR((ptr), (int)sizeof(BNU_CHUNK_T)) );
      ptr += prvExpLen*(Ipp32s)sizeof(BNU_CHUNK_T);

      RSA_PRV_KEY_NMONT(pKey) = (gsModEngine*)(ptr);
      ptr += montNsize;

      ZEXPAND_BNU(RSA_PRV_KEY_D(pKey), 0, prvExpLen);

      gsModEngineInit(RSA_PRV_KEY_NMONT(pKey), 0, rsaModulusBitSize, MOD_ENGINE_RSA_POOL_SIZE, gsModArithRSA());

      return ippStsNoErr;
   }
}
