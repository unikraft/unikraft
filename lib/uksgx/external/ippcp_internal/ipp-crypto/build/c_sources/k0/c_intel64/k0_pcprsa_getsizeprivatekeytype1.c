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
//        ippsRSA_GetSizePrivateKeyType1()
//
*/

#include "owncp.h"
#include "pcpbn.h"
#include "pcpngrsa.h"

#include "pcprsa_sizeof_privkey1.h"

/*F*
// Name: ippsRSA_GetSizePrivateKeyType1
//
// Purpose: Returns context size (bytes) of RSA private key (type1) context
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pSize
//
//    ippStsNotSupportedModeErr  MIN_RSA_SIZE > rsaModulusBitSize
//                               MAX_RSA_SIZE < rsaModulusBitSize
//
//    ippStsBadArgErr            0 >= privateExpBitSize
//                               privateExpBitSize > rsaModulusBitSize
//
//    ippStsNoErr                no error
//
// Parameters:
//    rsaModulusBitSize    bitsize of RSA modulus (bitsize of N)
//    privateExpBitSize    bitsize of private exponent (bitsize of D)
//    pSize                pointer to the size of RSA key context (bytes)
*F*/


IPPFUN(IppStatus, ippsRSA_GetSizePrivateKeyType1,(int rsaModulusBitSize, int privateExpBitSize, int* pKeySize))
{
   IPP_BAD_PTR1_RET(pKeySize);
   IPP_BADARG_RET((MIN_RSA_SIZE>rsaModulusBitSize) || (rsaModulusBitSize>MAX_RSA_SIZE), ippStsNotSupportedModeErr);
   IPP_BADARG_RET(!((0<privateExpBitSize) && (privateExpBitSize<=rsaModulusBitSize)), ippStsBadArgErr);

   //*pKeySize = cpSizeof_RSA_privateKey1(rsaModulusBitSize, privateExpBitSize);
   *pKeySize = cpSizeof_RSA_privateKey1(rsaModulusBitSize, rsaModulusBitSize);
   return ippStsNoErr;
}
