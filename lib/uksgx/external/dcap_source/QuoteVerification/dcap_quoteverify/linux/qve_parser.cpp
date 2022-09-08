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
/**
 * File: qve_parser.cpp
 *
 * Description: Wrapper functions for the
 * reference implementing the QvE
 * function defined in sgx_qve.h. This
 * would be replaced or used to wrap the
 * PSW defined interfaces to the QvE.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "se_trace.h"
#include "sgx_urts.h"
#include "sgx_dcap_pcs_com.h"

#define QvE_ENCLAVE_NAME "libsgx_qve.signed.so.1"
#define QvE_ENCLAVE_NAME_LEGACY "libsgx_qve.signed.so"

#ifndef MAX_PATH
#define MAX_PATH 260
#endif
static char g_qve_path[MAX_PATH];

extern "C" bool sgx_qv_set_qve_path(const char* p_path)
{
    // p_path isn't NULL, caller has checked it.
    // len <= sizeof(g_qve_path)
    size_t len = strnlen(p_path, sizeof(g_qve_path));
    // Make sure there is enough space for the '\0'
    // after this line len <= sizeof(g_qve_path) - 1
    if(len > sizeof(g_qve_path) - 1)
        return false;
    strncpy(g_qve_path, p_path, sizeof(g_qve_path) - 1);
    // Make sure the full path is ended with "\0"
    g_qve_path[len] = '\0';
    return true;
}


bool get_qve_path(
    char *p_file_path,
    size_t buf_size)
{
    if(!p_file_path)
        return false;

    Dl_info dl_info;
    if(g_qve_path[0])
    {
        strncpy(p_file_path, g_qve_path, buf_size -1);
        p_file_path[buf_size - 1] = '\0';  //null terminate the string
        return true;
    }
    else if(0 != dladdr(__builtin_return_address(0), &dl_info) &&
        NULL != dl_info.dli_fname)
    {
        if(strnlen(dl_info.dli_fname,buf_size)>=buf_size)
            return false;
        (void)strncpy(p_file_path,dl_info.dli_fname,buf_size);
    }
    else //not a dynamic executable
    {
        ssize_t i = readlink( "/proc/self/exe", p_file_path, buf_size );
        if (i == -1)
            return false;
        p_file_path[i] = '\0';
    }

    char* p_last_slash = strrchr(p_file_path, '/' );
    if ( p_last_slash != NULL )
    {
        p_last_slash++;   //increment beyond the last slash
        *p_last_slash = '\0';  //null terminate the string
    }
    else p_file_path[0] = '\0';
    if(strnlen(p_file_path,buf_size)+strnlen(QvE_ENCLAVE_NAME,buf_size)+sizeof(char)>buf_size)
        return false;
    (void)strncat(p_file_path,QvE_ENCLAVE_NAME, strnlen(QvE_ENCLAVE_NAME,buf_size));
    struct stat info;
    if(stat(p_file_path, &info) != 0 ||
        ((info.st_mode & S_IFREG) == 0 && (info.st_mode & S_IFLNK) == 0)) {
        if ( p_last_slash != NULL )
        {
            *p_last_slash = '\0';  //null terminate the string
        }
        else p_file_path[0] = '\0';
        (void)strncat(p_file_path, QvE_ENCLAVE_NAME_LEGACY, strnlen(QvE_ENCLAVE_NAME_LEGACY, buf_size));
    }
    return true;
}
