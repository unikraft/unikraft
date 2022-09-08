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


#ifndef _AESM_LOGIC_H_
#define _AESM_LOGIC_H_
#include "sgx_urts.h"
#include "aesm_error.h"
#include "arch.h"
#include "aeerror.h"
#include "tlv_common.h"
#include "se_thread.h"
#include "se_stdio.h"
#include "se_memcpy.h"
#include "uncopyable.h"
#include "oal/oal.h"
#include <time.h>
#include <string.h>
#include "se_wrapper.h"
#include "platform_info_blob.h"

#include "default_url_info.hh"

const uint32_t THREAD_TIMEOUT = 60000;   // milli-seconds

class AESMLogicMutex{
    CLASS_UNCOPYABLE(AESMLogicMutex)
public:
    AESMLogicMutex() {se_mutex_init(&mutex);}
    ~AESMLogicMutex() { se_mutex_destroy(&mutex);}
    void lock() { se_mutex_lock(&mutex); }
    void unlock() { se_mutex_unlock(&mutex); }
private:
    se_mutex_t mutex;
};

class AESMLogicLock {
    CLASS_UNCOPYABLE(AESMLogicLock)
public:
    explicit AESMLogicLock(AESMLogicMutex& cs) :_cs(cs) { _cs.lock(); }
    ~AESMLogicLock() { _cs.unlock(); }
private:
    AESMLogicMutex& _cs;
};

#define QE_PROD_ID  1
#define PSE_PROD_ID 2


typedef struct _endpoint_selection_infos_t endpoint_selection_infos_t;
#endif
