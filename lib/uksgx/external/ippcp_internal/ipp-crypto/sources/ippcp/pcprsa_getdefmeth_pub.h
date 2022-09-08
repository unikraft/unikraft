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

#include "owncp.h"
#include "pcpbn.h"
#include "pcpngrsa.h"
#include "pcpngrsamethod.h"


/* get default method based on CPU's features */
static gsMethod_RSA* getDefaultMethod_RSA_public(int modulusBitSize)
{
    gsMethod_RSA* m;

#if(_IPP32E>=_IPP32E_K1)
    m = IsFeatureEnabled(ippCPUID_AVX512IFMA) ? gsMethod_RSA_avx512_public() : gsMethod_RSA_avx2_public();
#elif(_IPP32E>=_IPP32E_L9)
    m = gsMethod_RSA_avx2_public();
#elif(_IPP>=_IPP_W7)
    m = gsMethod_RSA_sse2_public();
#else
    m = gsMethod_RSA_gpr_public();
#endif

    if (!(m->loModulusBisize <= modulusBitSize && modulusBitSize <= m->hiModulusBisize))
        m = gsMethod_RSA_gpr_public();
    return m;
}