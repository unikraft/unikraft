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
// Montgomery engine preparation (GetSize/init/Set)
*/

/*
//     Intel(R) Integrated Performance Primitives. Cryptography Primitives.
// 
//     Context:
//        gsMethod_RSA_gpr_private()
//
*/

#include "owncp.h"
#include "pcpngmontexpstuff.h"
#include "gsscramble.h"
#include "pcpngrsamethod.h"
#include "pcpngrsa.h"

/*
// Montgomery engine preparation (GetSize/init/Set)
*/
IPP_OWN_DEFN (void, rsaMontExpGetSize, (int maxLen32, int* pSize))
{
    int size = 0;
    int maxBitSize = maxLen32 << 5;
    gsModEngineGetSize(maxBitSize, MOD_ENGINE_RSA_POOL_SIZE, &size);

    *pSize = size;
}

