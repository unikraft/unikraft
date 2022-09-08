/*******************************************************************************
* Copyright 2019-2021 Intel Corporation
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

#ifndef IFMA_CP_LAYER_H
#define IFMA_CP_LAYER_H

#include <crypto_mb/defs.h>
#include <crypto_mb/rsa.h>

EXTERN_C void ifma_cp_rsa_pub_layer_mb8(const int8u* const from_pa[8],
                                              int8u* const to_pa[8],
                                        const int64u* const n_pa[8],
                                              int rsaBitlen,
                                        const mbx_RSA_Method* m,
                                              int8u* pBuffer);
EXTERN_C void ifma_cp_rsa_prv2_layer_mb8(const int8u* const from_pa[8],
                                               int8u* const to_pa[8],
                                         const int64u* const d_pa[8],
                                         const int64u* const n_pa[8],
                                               int rsaBitlen,
                                         const mbx_RSA_Method* m,
                                               int8u* pBuffer);
EXTERN_C void ifma_cp_rsa_prv5_layer_mb8(const int8u* const from_pa[8],
                                               int8u* const to_pa[8],
                                         const int64u* const p_pa[8],
                                         const int64u* const q_pa[8],
                                         const int64u* const dp_pa[8],
                                         const int64u* const dq_pa[8],
                                         const int64u* const iq_pa[8],
                                               int rsaBitlen,
                                         const mbx_RSA_Method* m,
                                               int8u* pBuffer);

#endif /* IFMA_CP_LAYER_H */
