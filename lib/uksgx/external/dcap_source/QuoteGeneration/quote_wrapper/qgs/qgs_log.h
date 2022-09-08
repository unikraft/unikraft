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

#ifndef __QGS_LOG_H__
#define __QGS_LOG_H__

#include "se_trace.h"
#define QGS_LOG_LEVEL_FATAL   0
#define QGS_LOG_LEVEL_ERROR   1
#define QGS_LOG_LEVEL_WARNING 2
#define QGS_LOG_LEVEL_INFO    3
#ifdef __cplusplus
extern "C" {
#endif/*__cplusplus*/
    void qgs_log_init(void);
    void qgs_log_init_ex(bool nosyslog);
    void qgs_log_fini(void);
#ifdef __cplusplus
};
#endif/*__cplusplus*/

#define QGS_LOG_FATAL(format, args...) \
    do {\
        sgx_proc_log_report(QGS_LOG_LEVEL_FATAL, format, ## args); \
    }while(0)
#define QGS_LOG_ERROR(format, args...) \
    do { \
        sgx_proc_log_report(QGS_LOG_LEVEL_ERROR, format, ## args); \
    }while(0);
#define QGS_LOG_WARN(format, args...)  \
    do { \
        sgx_proc_log_report(QGS_LOG_LEVEL_WARNING, format, ## args); \
    }while(0)
#define QGS_LOG_INFO(format, args...)  \
    do { \
        sgx_proc_log_report(QGS_LOG_LEVEL_INFO, format, ## args); \
    }while(0)
#define QGS_LOG_INIT() qgs_log_init()
#define QGS_LOG_INIT_EX(nosyslog) qgs_log_init_ex(nosyslog)
#define QGS_LOG_FINI() qgs_log_fini()

#endif/*__QGS_LOG_H__*/
