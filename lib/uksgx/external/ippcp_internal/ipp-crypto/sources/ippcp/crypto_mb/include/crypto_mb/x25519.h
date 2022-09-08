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

#ifndef X25519_H
#define X25519_H

#include <crypto_mb/defs.h>
#include <crypto_mb/status.h>

EXTERN_C mbx_status mbx_x25519_public_key_mb8(int8u* const pa_public_key[8],
                                     const int8u* const pa_private_key[8]);

EXTERN_C mbx_status mbx_x25519_mb8(int8u* const pa_shared_key[8],
                                const int8u* const pa_private_key[8],
                                const int8u* const pa_public_key[8]);

#endif /* X25519_H */
