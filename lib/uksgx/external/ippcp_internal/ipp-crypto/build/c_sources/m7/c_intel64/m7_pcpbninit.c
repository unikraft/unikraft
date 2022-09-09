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
//               Intel(R) Integrated Performance Primitives
//                   Cryptographic Primitives (ippcp)
//
//  Contents:
//     ippsBigNumGetSize()
//     ippsBigNumInit()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcptool.h"

/*F*
//    Name: ippsBigNumInit
//
// Purpose: Init BigNum spec for future usage.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pBN == NULL
//    ippStsLengthErr         length<1
//                            length > BITS2WORD32_SIZE(BN_MAXBITSIZE)
//    ippStsNoErr             no errors
//
// Parameters:
//    length    max BN length (32-bits segments)
//    pBN      BigNum ctx
//
*F*/
IPPFUN(IppStatus, ippsBigNumInit, (int length, IppsBigNumState* pBN))
{
    IPP_BADARG_RET(length<1 || length>BITS2WORD32_SIZE(BN_MAXBITSIZE), ippStsLengthErr);
    IPP_BAD_PTR1_RET(pBN);

    {
        Ipp8u* ptr = (Ipp8u*)pBN;

        /* convert length to the number of BNU_CHUNK_T */
        cpSize len = INTERNAL_BNU_LENGTH(length);

        BN_SIGN(pBN) = ippBigNumPOS;
        BN_SIZE(pBN) = 1;     /* initial valie is zero */
        BN_ROOM(pBN) = len;   /* close to what has been passed by user */

                              /* reserve one BNU_CHUNK_T more for cpDiv_BNU,
                              mul, mont exp operations */
        len++;

        ptr += sizeof(IppsBigNumState);

        /* allocate buffers */
        ptr = (Ipp8u*)(IPP_ALIGNED_PTR(ptr, BN_ALIGNMENT));
        BN_NUMBER(pBN) = (BNU_CHUNK_T*)ptr;
        ptr += len * (Ipp32s)sizeof(BNU_CHUNK_T);
        BN_BUFFER(pBN) = (BNU_CHUNK_T*)(ptr); /* use expanded length here */

                                                                           /* set BN value and buffer to zero */
        ZEXPAND_BNU(BN_NUMBER(pBN), 0, len);
        ZEXPAND_BNU(BN_BUFFER(pBN), 0, len);

        BN_SET_ID(pBN);
        return ippStsNoErr;
    }
}
