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
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcpprimeg.h"
#include "pcpprng.h"
#include "pcpngrsa.h"

/* test if E and A are coprime */
static int cpIsCoPrime(BNU_CHUNK_T* pA, int nsA,
    BNU_CHUNK_T* pB, int nsB,
    BNU_CHUNK_T* pBuffer)
{
    if (nsA>nsB) {
        SWAP_PTR(BNU_CHUNK_T, pA, pB);
        SWAP(nsA, nsB);
    }
    {
        __ALIGN8 IppsBigNumState bnA, bnB, bnGcd;
        BNU_CHUNK_T* pDataA = pBuffer;
        BNU_CHUNK_T* pBuffA = pDataA + nsA + 1;
        BNU_CHUNK_T* pDataB = pBuffA + nsA + 1;
        BNU_CHUNK_T* pBuffB = pDataB + nsB + 1;
        BNU_CHUNK_T* pDataGcd = pBuffB + nsB + 1;
        BNU_CHUNK_T* pBuffGcd = pDataGcd + nsB + 1;

        BN_Make(pDataA, pBuffA, nsA, &bnA);
        BN_Make(pDataB, pBuffB, nsB, &bnB);
        BN_Make(pDataGcd, pBuffGcd, nsB, &bnGcd);

        COPY_BNU(pDataA, pA, nsA)
            BN_Set(pDataA, nsA, &bnA);
        COPY_BNU(pDataB, pB, nsB)
            BN_Set(pDataB, nsB, &bnB);
        ippsGcd_BN(&bnA, &bnB, &bnGcd);
        return 0 == cpBN_cmp(&bnGcd, cpBN_OneRef());
    }
}