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
//        ippsRSA_GetSizePrivateKeyType2()
//
*/

#include "owncp.h"
#include "pcpbn.h"
#include "pcpngrsa.h"

#include "pcprsa_sizeof_privkey2.h"

/*F*
// Name: ippsRSA_GetSizePrivateKeyType2
//
// Purpose: Returns context size (bytes) of RSA private key (type2) context
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pSize
//
//    ippStsNotSupportedModeErr  MIN_RSA_SIZE > (factorPbitSize+factorQbitSize)
//                               MAX_RSA_SIZE < (factorPbitSize+factorQbitSize)
//
//    ippStsBadArgErr            0 >= factorPbitSize
//                               0 >= factorQbitSize
//                               factorQbitSize > factorPbitSize
//
//    ippStsNoErr                no error
//
// Parameters:
//    factorPbitSize    bitsize of RSA modulus (bitsize of P)
//    factorPbitSize    bitsize of private exponent (bitsize of Q)
//    pSize             pointer to the size of RSA key context (bytes)
*F*/


IPPFUN(IppStatus, ippsRSA_GetSizePrivateKeyType2,(int factorPbitSize, int factorQbitSize, int* pKeySize))
{
   IPP_BAD_PTR1_RET(pKeySize);
   IPP_BADARG_RET((factorPbitSize<=0) || (factorQbitSize<=0), ippStsBadArgErr);
   //25.09.2019 gres: IPP_BADARG_RET((factorPbitSize < factorQbitSize), ippStsBadArgErr);
   IPP_BADARG_RET((MIN_RSA_SIZE>(factorPbitSize+factorQbitSize) || (factorPbitSize+factorQbitSize)>MAX_RSA_SIZE), ippStsNotSupportedModeErr);

   *pKeySize = cpSizeof_RSA_privateKey2(factorPbitSize, factorQbitSize);
   return ippStsNoErr;
}
