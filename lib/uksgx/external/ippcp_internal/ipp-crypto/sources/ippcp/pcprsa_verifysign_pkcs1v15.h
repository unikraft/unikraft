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
//     RSASSA-PKCS-v1_5
//
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpngrsa.h"
#include "pcphash.h"
#include "pcptool.h"

#include "pcprsa_emsa_pkcs1v15.h"

static int VerifySign(const Ipp8u* pMsg, int msgLen,  /* message representation */
    const Ipp8u* pSalt, int saltLen, /* fied string */
    const Ipp8u* pSign,
    int* pIsValid,
    const IppsRSAPublicKeyState* pKey,
    BNU_CHUNK_T* pBuffer)
{
    /* size of RSA modulus in bytes and chunks */
    cpSize rsaBits = RSA_PUB_KEY_BITSIZE_N(pKey);
    cpSize k = BITS2WORD8_SIZE(rsaBits);
    cpSize nsN = BITS_BNU_CHUNK(rsaBits);

    /* temporary BNs */
    __ALIGN8 IppsBigNumState bnC;
    __ALIGN8 IppsBigNumState bnP;

    /* make BNs */
    BN_Make(pBuffer, pBuffer + nsN + 1, nsN, &bnC);
    pBuffer += (nsN + 1) * 2;
    BN_Make(pBuffer, pBuffer + nsN + 1, nsN, &bnP);
    pBuffer += (nsN + 1) * 2;

    /*
    // public-key operation
    */
    ippsSetOctString_BN(pSign, k, &bnP);
    gsRSApub_cipher(&bnC, &bnP, pKey, pBuffer);

    /* convert EM into the string */
    ippsGetOctString_BN((Ipp8u*)(BN_BUFFER(&bnC)), k, &bnC);

    /* EMSA-PKCS-v1_5 encoding */
    if (EMSA_PKCSv15(pMsg, msgLen, pSalt, saltLen, (Ipp8u*)(BN_NUMBER(&bnC)), k)) {
        *pIsValid = 1 == EquBlock((Ipp8u*)(BN_BUFFER(&bnC)), (Ipp8u*)(BN_NUMBER(&bnC)), k);
        return 1;
    }
    else
        return 0;
}
