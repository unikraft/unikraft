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

#ifndef ED25519_H
#define ED25519_H

#include <crypto_mb/defs.h>
#include <crypto_mb/status.h>

typedef int8u ed25519_sign_component[32];
typedef ed25519_sign_component ed25519_sign[2];

typedef int8u ed25519_public_key[32];
typedef int8u ed25519_private_key[32];

/*
// Computes ed25519 public key
// pa_public_key[]   array of pointers to the public keys
// pa_private_key[]  array of pointers to the public keys Y-coordinates
*/
EXTERN_C mbx_status mbx_ed25519_public_key_mb8(ed25519_public_key* pa_public_key[8],
                                         const ed25519_private_key* const pa_private_key[8]);

/*
// Computes ed25519 signature
// pa_sign_r[]       array of pointers to the computed r-components of the signatures
// pa_sign_s[]       array of pointers to the computed s-components of the signatures
// pa_msg[]          array of pointers to the messages are being signed
// msgLen[]          lengths of the messages are being signed
// pa_private_key[]  array of pointers to the signer's private keys
// pa_public_key[]   array of pointers to the signer's public keys
*/
EXTERN_C mbx_status mbx_ed25519_sign_mb8(ed25519_sign_component* pa_sign_r[8],
                                         ed25519_sign_component* pa_sign_s[8],
                                         const int8u* const pa_msg[8], const int32u msgLen[8],
                                         const ed25519_private_key* const pa_private_key[8],
                                         const ed25519_public_key* const pa_public_key[8]);

#endif /* ED25519_H */
