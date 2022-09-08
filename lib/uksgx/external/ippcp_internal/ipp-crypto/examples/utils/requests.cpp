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

/*!
  *
  * \file
  * \brief The source file contains the implementation of Request class which simulates a queue in the example of using the multi buffer RSA.
  *
  */

#include "bignum.h"
#include "requests.h"
#include "examples_common.h"

/* Allocated memory for cipher and decipher texts is not less than RSA modulus size and source plain text */
Request::Request(const BigNumber& pPlainText, const BigNumber& N, const BigNumber& E, const BigNumber& D)
    : m_plainText(pPlainText)
    , m_N(N)
    , m_E(E)
    , m_D(D)
    , m_cipherText(N)
    , m_decipherText(pPlainText)
    , m_isCompatible(true) {}

Request::~Request(){}
