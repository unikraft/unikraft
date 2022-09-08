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
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpngrsa.h"
#include "pcphash.h"
#include "pcptool.h"

#include "pcprsa_emsa_pkcs1v15.h"

static int GenerateSign(const Ipp8u* pMsg, int msgLen,  /* message representation */
    const Ipp8u* pSalt, int saltLen, /* fied string */
    Ipp8u* pSign,
    const IppsRSAPrivateKeyState* pPrvKey,
    const IppsRSAPublicKeyState*  pPubKey,
    BNU_CHUNK_T* pBuffer)
{
    /* size of RSA modulus in bytes and chunks */
    cpSize rsaBits = RSA_PRV_KEY_BITSIZE_N(pPrvKey);
    cpSize k = BITS2WORD8_SIZE(rsaBits);
    cpSize nsN = BITS_BNU_CHUNK(rsaBits);

    /* EMSA-PKCS-v1_5 encoding */
    int result = EMSA_PKCSv15(pMsg, msgLen, pSalt, saltLen, pSign, k);

    if (result) {
        /* temporary BNs */
        __ALIGN8 IppsBigNumState bnC;
        __ALIGN8 IppsBigNumState bnP;

        /* make BNs */
        BN_Make(pBuffer, pBuffer + nsN + 1, nsN, &bnC);
        pBuffer += (nsN + 1) * 2;
        BN_Make(pBuffer, pBuffer + nsN + 1, nsN, &bnP);
        pBuffer += (nsN + 1) * 2;

        /*
        // private-key operation
        */
        ippsSetOctString_BN(pSign, k, &bnC);

        if (RSA_PRV_KEY1_VALID_ID(pPrvKey))
            gsRSAprv_cipher(&bnP, &bnC, pPrvKey, pBuffer);
        else
            gsRSAprv_cipher_crt(&bnP, &bnC, pPrvKey, pBuffer);

        ippsGetOctString_BN(pSign, k, &bnP);

        /* check the result before send it out (fault attack mitigatioin) */
        if (pPubKey) {
            gsRSApub_cipher(&bnP, &bnP, pPubKey, pBuffer);

            /* check signature before send it out (fault attack mitigatioin) */
            if (0 != cpBN_cmp(&bnP, &bnC)) {
                PadBlock(0, pSign, k);
                result = 0;
            }
        }
    }

    return result;
}
