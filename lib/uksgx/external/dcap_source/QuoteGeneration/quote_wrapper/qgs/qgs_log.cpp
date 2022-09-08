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

#include <syslog.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "qgs_log.h"

static bool _nosyslog = false;

void qgs_log_init(void)
{
    if (!_nosyslog) {
        openlog("qgsd", LOG_CONS|LOG_PID, LOG_USER);
    }
}

void qgs_log_init_ex(bool nosyslog)
{
    // If nosyslog is true, we will output logs to stdout instead of syslog
    _nosyslog = nosyslog;

    qgs_log_init();
}

void qgs_log_fini(void)
{
    if (!_nosyslog) {
        closelog();
    }
}

extern "C"
void sgx_proc_log_report(int level, const char *format, ...)
{
    int priority = 0;
    va_list ap;
    // Make sure strlen(format) >= 1
    // so we can always add newline
    if (!format || !(*format))
        return;//ignore
    va_start(ap, format);
    switch(level){
        case QGS_LOG_LEVEL_FATAL:
            priority = LOG_CRIT;
            break;
        case QGS_LOG_LEVEL_ERROR:
            priority = LOG_ERR;
            break;
        case QGS_LOG_LEVEL_WARNING:
            priority = LOG_WARNING;
            break;
        case QGS_LOG_LEVEL_INFO:
            priority = LOG_INFO;
            break;
        default:
            return;//ignore
    }
    if (!_nosyslog) {
        vsyslog(priority, format, ap);
    }
    else {
        FILE* stream = nullptr;
        if (level <= QGS_LOG_LEVEL_ERROR) {
            stream = stderr;
        }
        else {
            stream = stdout;
        }
        // Automatically add newline
        vfprintf(stream, format, ap);
        if (format[strlen(format)-1] != '\n')
            fprintf(stream, "\n");
        fflush(stream);
    }
    va_end(ap);
}
