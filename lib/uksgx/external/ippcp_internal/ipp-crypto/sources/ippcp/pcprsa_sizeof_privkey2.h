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
*/

static int cpSizeof_RSA_privateKey2(int factorPbitSize, int factorQbitSize)
{
    int factorPlen = BITS_BNU_CHUNK(factorPbitSize);
    int factorQlen = BITS_BNU_CHUNK(factorQbitSize);
    int factorPlen32 = BITS2WORD32_SIZE(factorPbitSize);
    int factorQlen32 = BITS2WORD32_SIZE(factorQbitSize);
    int rsaModulusLen32 = BITS2WORD32_SIZE(factorPbitSize + factorQbitSize);
    int montPsize;
    int montQsize;
    int montNsize;
    rsaMontExpGetSize(factorPlen32, &montPsize);
    rsaMontExpGetSize(factorQlen32, &montQsize);
    rsaMontExpGetSize(rsaModulusLen32, &montNsize);

    return (Ipp32s)sizeof(IppsRSAPrivateKeyState)
        + factorPlen * (Ipp32s)sizeof(BNU_CHUNK_T)  /* dp slot */
        + factorQlen * (Ipp32s)sizeof(BNU_CHUNK_T)  /* dq slot */
        + factorPlen * (Ipp32s)sizeof(BNU_CHUNK_T)  /* qinv slot */
        + (Ipp32s)sizeof(BNU_CHUNK_T) - 1           /* alignment */ 
        + montPsize
        + montQsize
        + montNsize;
}
