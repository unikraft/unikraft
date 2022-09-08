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

#include "qgs/qgs.message.pb-c.h"
#include <sys/socket.h>
#include <linux/vm_sockets.h>
#include "tdx_attest.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/types.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <fcntl.h>
// For strtoul
#include <limits.h>
#include <errno.h>
#include <syslog.h>

#define TDX_ATTEST_DEV_PATH "/dev/tdx-attest"
#define CFG_FILE_PATH "/etc/tdx-attest.conf"
//TODO: Should include kernel header
#define TDX_CMD_GET_TDREPORT _IOWR('T', 0x01, __u64)
#define TDX_CMD_EXTEND_RTMR _IOR('T', 0x04, __u64)
#define TDX_CMD_GET_EXTEND_SIZE	_IOR('T', 0x05, __u64)
#define TDX_CMD_GEN_QUOTE _IOR('T', 0x02, __u64)

#ifdef DEBUG
#define TDX_TRACE                                          \
    do {                                                   \
        fprintf(stderr, "\n[%s:%d] ", __FILE__, __LINE__); \
        perror(NULL);                                      \
    }while(0)
#else
#define TDX_TRACE
#endif

#pragma pack(push, 1)
// It's a 4*4K byte structure
typedef struct _get_quote_blob_t
{
    // Following fields are used for transport layer, LE
    uint64_t version;
    uint64_t status;
    uint32_t in_len;
    uint32_t out_len;
     // Following fields are consumed by QGS and this library
    uint8_t trans_len[4];   // BE
    uint8_t p_buf[4 * 4 * 1024 - 28];
} get_quote_blob_t;

typedef struct _get_quote_ioctl_arg_t
{
    void *p_blob;
    size_t len;
} get_quote_ioctl_arg_t;
#pragma pack(pop)

static const unsigned HEADER_SIZE = 4;
static const tdx_uuid_t g_intel_tdqe_uuid = {TDX_SGX_ECDSA_ATTESTATION_ID};

static unsigned int get_vsock_port(void)
{
    FILE *p_config_fd = NULL;
    char *p_line = NULL;
    char *p = NULL;
    size_t line_len = 0;
    long long_num = 0;
    unsigned int port = 0;

    p_config_fd = fopen(CFG_FILE_PATH, "r");
    if (NULL == p_config_fd) {
        TDX_TRACE;
        return 0;
    }
    while(-1 != getline(&p_line, &line_len, p_config_fd)) {
        char temp[11] = {0};
        int number = 0;
        int ret = sscanf(p_line, " %10[#]", temp);
        if (ret == 1) {
            continue;
        }
        /* leading or trailing white space are ignored, white space around '='
           are also ignored. The number should no longer than 10 characters.
           Trailing non-whitespace are not allowed. */
        ret = sscanf(p_line, " port = %10[0-9] %n", temp, &number);
        /* Make sure number is positive then make the cast. It's not likely to
           have a negtive value, just a defense-in-depth. The cast is used to
           suppress the -Wsign-compare warning. */
        if (ret == 1 && number > 0 && ((size_t)number < line_len)
            && !p_line[number]) {
            errno = 0;
            long_num = strtol(temp, &p, 10);
            if (p == temp) {
                TDX_TRACE;
                port = 0;
                break;
            }

            // make sure that no range error occurred
            if (errno == ERANGE || long_num > UINT_MAX) {
                TDX_TRACE;
                port = 0;
                break;
            }

            // range is ok, so we can convert to int
            port = (unsigned int)long_num & 0xFFFFFFFF;
            #ifdef DBUG
            fprintf(stdout, "\nGet the vsock port number [%u]\n", port);
            #endif
            break;
        }
    }

    /* p_line is allocated by sscanf */
    free(p_line);
    fclose(p_config_fd);

    return port;
}

static tdx_attest_error_t get_tdx_report(
    int devfd,
    const tdx_report_data_t *p_tdx_report_data,
    tdx_report_t *p_tdx_report)
{
    if (-1 == devfd) {
        return TDX_ATTEST_ERROR_UNEXPECTED;
    }
    if (!p_tdx_report) {
        fprintf(stderr, "\nNeed to input TDX report.");
        return TDX_ATTEST_ERROR_INVALID_PARAMETER;
    }

    uint8_t tdx_report[TDX_REPORT_SIZE] = {0};
    if (p_tdx_report_data) {
        memcpy(tdx_report, p_tdx_report_data->d, sizeof(p_tdx_report_data->d));
    }

    if (-1 == ioctl(devfd, TDX_CMD_GET_TDREPORT, tdx_report)) {
        TDX_TRACE;
        return TDX_ATTEST_ERROR_REPORT_FAILURE;
    }
    memcpy(p_tdx_report->d, tdx_report, sizeof(p_tdx_report->d));
    return TDX_ATTEST_SUCCESS;
}

tdx_attest_error_t tdx_att_get_quote(
    const tdx_report_data_t *p_tdx_report_data,
    const tdx_uuid_t *p_att_key_id_list,
    uint32_t list_size,
    tdx_uuid_t *p_att_key_id,
    uint8_t **pp_quote,
    uint32_t *p_quote_size,
    uint32_t flags)
{
    int s = -1;
    int devfd = -1;
    int use_tdvmcall = 1;
    uint32_t quote_size = 0;
    uint32_t recieved_bytes = 0;
    uint32_t in_msg_size = 0;
    unsigned int vsock_port = 0;
    tdx_attest_error_t ret = TDX_ATTEST_ERROR_UNEXPECTED;
    get_quote_blob_t *p_get_quote_blob = NULL;
    tdx_report_t tdx_report;
    uint32_t msg_size = 0;
    Qgs__Message__Response *resp;
    Qgs__Message__Request request = QGS__MESSAGE__REQUEST__INIT;
    Qgs__Message__Request__GetQuoteRequest get_quote_request =
        QGS__MESSAGE__REQUEST__GET_QUOTE_REQUEST__INIT;

    if ((!p_att_key_id_list && list_size) ||
        (p_att_key_id_list && !list_size)) {
        ret = TDX_ATTEST_ERROR_INVALID_PARAMETER;
        goto ret_point;
    }
    if (!pp_quote) {
        ret = TDX_ATTEST_ERROR_INVALID_PARAMETER;
        goto ret_point;
    }
    if (flags) {
        //TODO: I think we need to have a runtime version to make this flag usable.
        ret = TDX_ATTEST_ERROR_INVALID_PARAMETER;
        goto ret_point;
    }

    // Currently only intel TDQE are supported
    if (1 < list_size) {
        ret = TDX_ATTEST_ERROR_INVALID_PARAMETER;
    }
    if (p_att_key_id_list && memcmp(p_att_key_id_list, &g_intel_tdqe_uuid,
                    sizeof(g_intel_tdqe_uuid))) {
        ret = TDX_ATTEST_ERROR_INVALID_PARAMETER;
    }
    *pp_quote = NULL;
    memset(&tdx_report, 0, sizeof(tdx_report));
    p_get_quote_blob = (get_quote_blob_t *)malloc(sizeof(get_quote_blob_t));
    if (!p_get_quote_blob) {
        ret = TDX_ATTEST_ERROR_OUT_OF_MEMORY;
        goto ret_point;
    }

    devfd = open(TDX_ATTEST_DEV_PATH, O_RDWR | O_SYNC);
    if (-1 == devfd) {
        TDX_TRACE;
        ret = TDX_ATTEST_ERROR_DEVICE_FAILURE;
        goto ret_point;
    }

    ret = get_tdx_report(devfd, p_tdx_report_data, &tdx_report);
    if (TDX_ATTEST_SUCCESS != ret) {
        goto ret_point;
    }

    request.type = QGS__MESSAGE__REQUEST__MSG_GET_QUOTE_REQUEST;
    get_quote_request.report.len = sizeof(tdx_report.d);
    get_quote_request.report.data = tdx_report.d;
    request.msg_case = QGS__MESSAGE__REQUEST__MSG_GET_QUOTE_REQUEST;
    request.getquoterequest = &get_quote_request;

    // Add the size header
    msg_size = (uint32_t)qgs__message__request__get_packed_size(&request);
    p_get_quote_blob->trans_len[0] = (uint8_t)((msg_size >> 24) & 0xFF);
    p_get_quote_blob->trans_len[1] = (uint8_t)((msg_size >> 16) & 0xFF);
    p_get_quote_blob->trans_len[2] = (uint8_t)((msg_size >> 8) & 0xFF);
    p_get_quote_blob->trans_len[3] = (uint8_t)(msg_size & 0xFF);

    // Serialization
    qgs__message__request__pack(&request, p_get_quote_blob->p_buf);

    do {
        vsock_port = get_vsock_port();
        if (!vsock_port) {
            syslog(LOG_INFO, "libtdx_attest: fallback to tdvmcall mode.");
            break;
        }
        s = socket(AF_VSOCK, SOCK_STREAM, 0);
        if (-1 == s) {
            syslog(LOG_INFO, "libtdx_attest: fallback to tdvmcall mode.");
            break;
        }
        struct sockaddr_vm vm_addr;
        memset(&vm_addr, 0, sizeof(vm_addr));
        vm_addr.svm_family = AF_VSOCK;
        vm_addr.svm_reserved1 = 0;
        vm_addr.svm_port = vsock_port;
        vm_addr.svm_cid = VMADDR_CID_HOST;
        if (connect(s, (struct sockaddr *)&vm_addr, sizeof(vm_addr))) {
            syslog(LOG_INFO, "libtdx_attest: fallback to tdvmcall mode.");
            break;
        }

        // Write to socket
        if (HEADER_SIZE + msg_size != send(s, p_get_quote_blob->trans_len,
            HEADER_SIZE + msg_size, 0)) {
            TDX_TRACE;
            ret = TDX_ATTEST_ERROR_VSOCK_FAILURE;
            goto ret_point;
        }

        // Read the response size header
        if (HEADER_SIZE != recv(s, p_get_quote_blob->trans_len,
            HEADER_SIZE, 0)) {
            TDX_TRACE;
            ret = TDX_ATTEST_ERROR_VSOCK_FAILURE;
            goto ret_point;
        }

        // decode the size
        for (unsigned i = 0; i < HEADER_SIZE; ++i) {
            in_msg_size = in_msg_size * 256
                          + ((p_get_quote_blob->trans_len[i]) & 0xFF);
        }

        // prepare the buffer and read the reply body
        #ifdef DEBUG
        fprintf(stdout, "\nReply message body is %u bytes", in_msg_size);
        #endif

        if (sizeof(p_get_quote_blob->p_buf) < in_msg_size)
        {
            #ifdef DEBUG
            fprintf(stdout, "\nReply message body is too big");
            #endif
            ret = TDX_ATTEST_ERROR_UNEXPECTED;
            goto ret_point;
        }
        while( recieved_bytes < in_msg_size) {
            int recv_ret = (int)recv(s, p_get_quote_blob->p_buf + recieved_bytes,
                                     in_msg_size - recieved_bytes, 0);
            if (recv_ret < 0) {
                ret = TDX_ATTEST_ERROR_VSOCK_FAILURE;
                goto ret_point;
            }
            recieved_bytes += (uint32_t)recv_ret;
        }
        #ifdef DEBUG
        fprintf(stdout, "\nGet %u bytes response from vsock", recieved_bytes);
        #endif
        use_tdvmcall = 0;
    } while (0);

    if (use_tdvmcall) {
        int ioctl_ret = 0;
        get_quote_ioctl_arg_t arg;
        p_get_quote_blob->version = 1;
        p_get_quote_blob->status = 0;
        p_get_quote_blob->in_len = HEADER_SIZE + msg_size;
        p_get_quote_blob->out_len = (uint32_t)(sizeof(*p_get_quote_blob) - 24);
        arg.p_blob = p_get_quote_blob;
        arg.len = sizeof(*p_get_quote_blob);

        ioctl_ret = ioctl(devfd, TDX_CMD_GEN_QUOTE, &arg);
        if (EBUSY == ioctl_ret) {
            TDX_TRACE;
            ret = TDX_ATTEST_ERROR_BUSY;
            goto ret_point;
        } else if (ioctl_ret) {
            TDX_TRACE;
            ret = TDX_ATTEST_ERROR_QUOTE_FAILURE;
            goto ret_point;
        }
        if (p_get_quote_blob->status
            || p_get_quote_blob->out_len <= HEADER_SIZE) {
            TDX_TRACE;
            ret = TDX_ATTEST_ERROR_UNEXPECTED;
            goto ret_point;
        }

        //in_msg_size is the size of serialized response, remove 4bytes header
        //TODO: Decode the HEAD and compare it with out_len as defense-in-depth
        in_msg_size = p_get_quote_blob->out_len - HEADER_SIZE;
        #ifdef DEBUG
        fprintf(stdout, "\nGet %u bytes response from tdvmcall", in_msg_size);
        #endif
    }

    resp = qgs__message__response__unpack(
        NULL, in_msg_size, p_get_quote_blob->p_buf);
    if (!resp) {
        ret = TDX_ATTEST_ERROR_UNEXPECTED;
        goto ret_point;
    }

    switch (resp->type)
    {
    case QGS__MESSAGE__RESPONSE__MSG_GET_QUOTE_RESPONSE:
        if (resp->getquoteresponse->error_code != 0) {
            ret = TDX_ATTEST_ERROR_UNEXPECTED;
            goto ret_point;
        }
        quote_size = (uint32_t)resp->getquoteresponse->quote.len;
        *pp_quote = malloc(quote_size);
        if (!*pp_quote) {
            ret = TDX_ATTEST_ERROR_OUT_OF_MEMORY;
            goto ret_point;
        }
        memcpy(*pp_quote, resp->getquoteresponse->quote.data, quote_size);
        if (p_quote_size) {
            *p_quote_size = quote_size;
        }
        if (p_att_key_id) {
            *p_att_key_id = g_intel_tdqe_uuid;
        }
        break;
    default:
        ret = TDX_ATTEST_ERROR_UNEXPECTED;
    }

ret_point:
    if (s >= 0) {
        close(s);
    }
    if (-1 != devfd) {
        close(devfd);
    }
    free(p_get_quote_blob);

    return ret;
}

tdx_attest_error_t tdx_att_free_quote(
    uint8_t *p_quote)
{
    free(p_quote);
    return TDX_ATTEST_SUCCESS;
}

tdx_attest_error_t tdx_att_get_report(
    const tdx_report_data_t *p_tdx_report_data,
    tdx_report_t *p_tdx_report)
{
    int devfd;
    tdx_attest_error_t ret = TDX_ATTEST_SUCCESS;

    devfd = open(TDX_ATTEST_DEV_PATH, O_RDWR | O_SYNC);
    if (-1 == devfd) {
        TDX_TRACE;
        return TDX_ATTEST_ERROR_DEVICE_FAILURE;
    }

    ret = get_tdx_report(devfd, p_tdx_report_data, p_tdx_report);

    close(devfd);
    return ret;
}

tdx_attest_error_t tdx_att_get_supported_att_key_ids(
        tdx_uuid_t *p_att_key_id_list,
        uint32_t *p_list_size)
{
    if (!p_list_size) {
        return TDX_ATTEST_ERROR_INVALID_PARAMETER;
    }
    if (p_att_key_id_list && !*p_list_size) {
        return TDX_ATTEST_ERROR_INVALID_PARAMETER;
    }
    if (!p_att_key_id_list && *p_list_size) {
        return TDX_ATTEST_ERROR_INVALID_PARAMETER;
    }
    if (p_att_key_id_list) {
        p_att_key_id_list[0] = g_intel_tdqe_uuid;
    }
    *p_list_size = 1;
    return TDX_ATTEST_SUCCESS;
}

tdx_attest_error_t tdx_att_extend(
    const tdx_rtmr_event_t *p_rtmr_event)
{
    int devfd = -1;
    uint64_t extend_data_size = 0;
    if (!p_rtmr_event || p_rtmr_event->version != 1) {
        return TDX_ATTEST_ERROR_INVALID_PARAMETER;
    }
    if (p_rtmr_event->event_data_size) {
        return TDX_ATTEST_ERROR_NOT_SUPPORTED;
    }

    devfd = open(TDX_ATTEST_DEV_PATH, O_RDWR | O_SYNC);
    if (-1 == devfd) {
        TDX_TRACE;
        return TDX_ATTEST_ERROR_DEVICE_FAILURE;
    }

    if (-1 == ioctl(devfd, TDX_CMD_GET_EXTEND_SIZE, &extend_data_size)) {
        TDX_TRACE;
        close(devfd);
        return TDX_ATTEST_ERROR_EXTEND_FAILURE;
    }
    assert(extend_data_size == sizeof(p_rtmr_event->extend_data));
    if (-1 == ioctl(devfd, TDX_CMD_EXTEND_RTMR, &p_rtmr_event->rtmr_index)) {
        TDX_TRACE;
        close(devfd);
        if (EINVAL == errno) {
            return TDX_ATTEST_ERROR_INVALID_RTMR_INDEX;
        }
        return TDX_ATTEST_ERROR_EXTEND_FAILURE;
    }
    close(devfd);
    return TDX_ATTEST_SUCCESS;
}
