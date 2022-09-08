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

#include "Queue.h"
#include "Queue.tmh"

#include "driver.h"
#include "sgx_lc_msr_public.h"

extern sgx_get_launch_support_output_t launch_support_info;

NTSTATUS
FLCMSRQueueInitialize(
    _In_ WDFDEVICE Device
)
{
    WDFQUEUE queue;
    NTSTATUS status;
    WDF_IO_QUEUE_CONFIG queueConfig;
    //
    // Configure a default queue so that requests that are not
    // configure-fowarded using WdfDeviceConfigureRequestDispatching to goto
    // other queues get dispatched here.
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
        &queueConfig,
        WdfIoQueueDispatchParallel
    );

    queueConfig.EvtIoDeviceControl = FLCMSREvtIoDeviceControl;
    queueConfig.EvtIoStop = FLCMSREvtIoStop;
    status = WdfIoQueueCreate(
                 Device,
                 &queueConfig,
                 WDF_NO_OBJECT_ATTRIBUTES,
                 &queue
             );

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "WdfIoQueueCreate failed %!STATUS!", status);
        return status;
    }

    return status;
}

VOID
FLCMSREvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
)
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    size_t buffer_size = 0;

    TraceEvents(TRACE_LEVEL_INFORMATION,
                TRACE_QUEUE,
                "%!FUNC! Queue 0x%p, Request 0x%p OutputBufferLength %d InputBufferLength %d IoControlCode %d",
                Queue, Request, (int)OutputBufferLength, (int)InputBufferLength, IoControlCode);

    switch (IoControlCode)
    {

    case IOCTL_SGX_GETLAUNCHSUPPORT:
    {
        WDFMEMORY memory = NULL;

        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "IOCTL_SGX_GETLAUNCHSUPPORT");

        status = WdfRequestRetrieveInputMemory(Request, &memory);
        if (status != STATUS_SUCCESS)
            break;

        sgx_get_launch_support_input_t* buffer_in = (sgx_get_launch_support_input_t*)WdfMemoryGetBuffer(memory, &buffer_size);
        if (buffer_in == NULL || buffer_size < sizeof(sgx_get_launch_support_input_t) || buffer_in->version != 0)
        {
            buffer_size = 0;
            break;
        }

        status = WdfRequestRetrieveOutputMemory(Request, &memory);
        if (status != STATUS_SUCCESS)
            break;

        sgx_get_launch_support_output_t* buffer_out = (sgx_get_launch_support_output_t*)WdfMemoryGetBuffer(memory, &buffer_size);
        if (buffer_out == NULL || buffer_size < sizeof(sgx_get_launch_support_output_t))
        {
            buffer_size = 0;
            break;
        }

        errno_t err = memcpy_s(buffer_out, buffer_size, &launch_support_info, sizeof(sgx_get_launch_support_output_t));

        if (err)
            break;

        buffer_size = sizeof(sgx_get_launch_support_output_t);
        status = STATUS_SUCCESS;
    }
    default:
        break;
    }

    WdfRequestCompleteWithInformation(Request, status, buffer_size);
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "%!FUNC! Exit");
    return;
}

VOID
FLCMSREvtIoStop(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ ULONG ActionFlags
)

{
    TraceEvents(TRACE_LEVEL_INFORMATION,
                TRACE_QUEUE,
                "%!FUNC! Queue 0x%p, Request 0x%p ActionFlags %d",
                Queue, Request, ActionFlags);
    return;
}
