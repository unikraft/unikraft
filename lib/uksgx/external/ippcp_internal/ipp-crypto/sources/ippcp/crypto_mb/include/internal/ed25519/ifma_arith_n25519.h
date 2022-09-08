/*******************************************************************************
* Copyright 2021-2021 Intel Corporation
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

#ifndef IFMA_ARITH_N25519_H
#define IFMA_ARITH_N25519_H

#include <internal/common/ifma_defs.h>
#include <internal/common/ifma_math.h>

/* bitsize of base point order */
#define N25519_BITSIZE  (253)
#define NE_LEN52        NUMBER_OF_DIGITS(N25519_BITSIZE, DIGIT_SIZE)
#define NE_LEN64        NUMBER_OF_DIGITS(N25519_BITSIZE, 64)

void ifma52_ed25519n_madd(U64 r[NE_LEN52], const U64 a[NE_LEN52], const U64 b[NE_LEN52], const U64 c[NE_LEN52]);
void ifma52_ed25519n_reduce(U64 r[NE_LEN52], const U64 x[NE_LEN52 * 2]);

#endif /* IFMA_ARITH_N25519_H */
