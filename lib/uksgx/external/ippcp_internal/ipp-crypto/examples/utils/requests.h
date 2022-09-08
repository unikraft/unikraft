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
  * \brief The header contains the declaration of Request class which simulates a queue in the example of using the multi buffer RSA.
  *
  */

#if !defined _REQUESTS_H_
#define _REQUESTS_H_

#include "bignum.h"

class Request {
    BigNumber m_plainText;
    BigNumber m_cipherText;
    BigNumber m_decipherText;
    BigNumber m_N;
    BigNumber m_E;
    BigNumber m_D;
    bool m_isCompatible;

    public:
        Request(const BigNumber& pPlainText, const BigNumber& N, const BigNumber& E, const BigNumber& D);
        ~Request();

        IppsBigNumState* GetPlainText() const
        {
            return m_plainText;
        }

        IppsBigNumState* GetCipherText() const
        {
            return m_cipherText;
        }

        IppsBigNumState* GetDecipherText() const
        {
            return m_decipherText;
        }

        int GetBitSizeN() const
        {
            return m_N.BitSize();
        }

        const BigNumber& GetValueN() const
        {
            return m_N;
        }

        int GetBitSizeE() const
        {
            return m_E.BitSize();
        }

        const BigNumber& GetValueE() const
        {
            return m_E;
        }

        int GetBitSizeD() const
        {
            return m_D.BitSize();
        }

        const BigNumber& GetValueD() const
        {
            return m_D;
        }
        void SetCompatibilityStatus(bool status)
        {
            m_isCompatible = status;
        }

        bool IsCompatible() const
        {
            return m_isCompatible;
        }        
};

#endif /* #ifndef _REQUESTS_H_ */
