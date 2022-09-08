/*
 * Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "SgxEcdsaAttestation/AttestationParsers.h"
#include "TcbTestUtils.h"

#include <gtest/gtest.h>

using namespace intel::sgx::dcap::parser::x509;
using namespace intel::sgx::dcap::parser;
using namespace ::testing;

struct TcbUT: public testing::Test {};

TEST_F(TcbUT, tcbConstructors)
{
    ASSERT_NO_THROW(createTcb());
}

TEST_F(TcbUT, tcbGetters)
{
    const auto& tcb = createTcb();

    for (unsigned long i=0; i < 16; i++)
    {
        ASSERT_EQ(tcb.getSgxTcbComponentSvn((uint32_t)i), CPUSVN_COMPONENTS[i]);
    }
    ASSERT_EQ(tcb.getPceSvn(), PCESVN);
    ASSERT_EQ(tcb.getCpuSvn(), CPUSVN);
    ASSERT_EQ(tcb.getSgxTcbComponents(), CPUSVN_COMPONENTS);
    ASSERT_EQ(tcb.getSgxTcbComponents(), tcb.getCpuSvn());
}

TEST_F(TcbUT, tcbOperators)
{
    const auto& tcb = createTcb();

    ASSERT_EQ(tcb, createTcb());
    ASSERT_FALSE(tcb == createTcb({}));
    ASSERT_FALSE(tcb == createTcb(CPUSVN, {}));
    ASSERT_FALSE(tcb == createTcb(CPUSVN, CPUSVN_COMPONENTS, 1234));
}
