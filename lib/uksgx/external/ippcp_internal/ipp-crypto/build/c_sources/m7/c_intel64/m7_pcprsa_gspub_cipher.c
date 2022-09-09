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
//        gsRSApub_cipher()
//
*/

#include "owncp.h"
#include "pcpbn.h"
#include "pcpngrsa.h"
#include "pcpngrsamethod.h"

#include "pcprsa_getdefmeth_pub.h"

IPP_OWN_DEFN (void, gsRSApub_cipher, (IppsBigNumState* pY, const IppsBigNumState* pX, const IppsRSAPublicKeyState* pKey, BNU_CHUNK_T* pBuffer))
{
   gsMethod_RSA* m = getDefaultMethod_RSA_public(RSA_PRV_KEY_BITSIZE_N(pKey));

   BNU_CHUNK_T* dataY = BN_NUMBER(pY);
   cpSize nsY = m->expFun(dataY,
                          BN_NUMBER(pX), BN_SIZE(pX),
                          RSA_PUB_KEY_E(pKey), RSA_PUB_KEY_BITSIZE_E(pKey),
                          RSA_PUB_KEY_NMONT(pKey),
                          pBuffer);
   FIX_BNU(dataY, nsY);
   BN_SIZE(pY) = nsY;
   BN_SIGN(pY) = ippBigNumPOS;
}
