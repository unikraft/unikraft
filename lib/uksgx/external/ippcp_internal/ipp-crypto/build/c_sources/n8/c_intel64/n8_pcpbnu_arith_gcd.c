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
//  Purpose:
//     Intel(R) Integrated Performance Primitives. Cryptography Primitives.
//     Internal Unsigned arithmetic
// 
//  Contents:
//     cpGcd_BNU()
// 
*/

#include "owncp.h"
#include "pcpbnuarith.h"
#include "pcpbnumisc.h"

/*F*
//    Name: cpGcd_BNU
//
// Purpose: compute GCD value.
//
// Returns:
//    GCD value
//
// Parameters:
//    a    source BigNum
//    b    source BigNum
//
*F*/

IPP_OWN_DEFN (BNU_CHUNK_T, cpGcd_BNU, (BNU_CHUNK_T a, BNU_CHUNK_T b))
{
    BNU_CHUNK_T gcd, t, r;

    if(a > b){
        gcd = a;
        t = b;
    } else {
        t = a;
        gcd = b;
    }

    while (t != 0)    {
        r = gcd % t;
        gcd = t;
        t = r;
    }
    return gcd;
}
