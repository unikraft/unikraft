/*******************************************************************************
* Copyright 2016-2021 Intel Corporation
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
*/
#if !defined(_CP_MONTRED_H)
#define _CP_MONTRED_H

#include "owndefs.h"
#include "owncp.h"

#include "pcpbnuimpl.h"
/*
// Montgomery reduction
*/
#define cpMontRedAdc_BNU OWNAPI(cpMontRedAdc_BNU)
    IPP_OWN_DECL (void, cpMontRedAdc_BNU, (BNU_CHUNK_T* pR, BNU_CHUNK_T* pProduct, const BNU_CHUNK_T* pModulus, cpSize nsM, BNU_CHUNK_T m0))
#define cpMontRedAdx_BNU OWNAPI(cpMontRedAdx_BNU)
    IPP_OWN_DECL (void, cpMontRedAdx_BNU, (BNU_CHUNK_T* pR, BNU_CHUNK_T* pProduct, const BNU_CHUNK_T* pModulus, cpSize nsM, BNU_CHUNK_T m0))

__INLINE void cpMontRed_BNU_opt(BNU_CHUNK_T* pR,
                                BNU_CHUNK_T* pProduct,
                          const BNU_CHUNK_T* pModulus, cpSize nsM, BNU_CHUNK_T m0)
{
#if(_ADCOX_NI_ENABLING_==_FEATURE_ON_)
   cpMontRedAdx_BNU(pR, pProduct, pModulus, nsM, m0);
#elif(_ADCOX_NI_ENABLING_==_FEATURE_TICKTOCK_)
   IsFeatureEnabled(ippCPUID_ADCOX)? cpMontRedAdx_BNU(pR, pProduct, pModulus, nsM, m0)
                                   : cpMontRedAdc_BNU(pR, pProduct, pModulus, nsM, m0);
#else
   cpMontRedAdc_BNU(pR, pProduct, pModulus, nsM, m0);
#endif
}

#endif /* _CP_MONTRED_H */
