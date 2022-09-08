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

static int EMSA_PKCSv15(const Ipp8u* msgDg, int lenMsgDg,
    const Ipp8u* fixPS, int lenFixPS,
    Ipp8u*   pEM, int lenEM)
{
    /*
    // encoded message format:
    //    EM = 00 || 01 || PS=(FF..FF) || 00 || T
    //    T = fixPS || msgDg
    //    len(PS) >= 8
    */
    int  tLen = lenFixPS + lenMsgDg;

    if (lenEM >= tLen + 11) {
        int psLen = lenEM - 3 - tLen;

        PadBlock(0xFF, pEM, lenEM);
        pEM[0] = 0x00;
        pEM[1] = 0x01;
        pEM[2 + psLen] = 0x00;
        CopyBlock(fixPS, pEM + 3 + psLen, lenFixPS);
        if (msgDg) {
           CopyBlock(msgDg, pEM + 3 + psLen + lenFixPS, lenMsgDg);
        }
        return 1;
    }
    else
        return 0; /* encoded message length too long */
}