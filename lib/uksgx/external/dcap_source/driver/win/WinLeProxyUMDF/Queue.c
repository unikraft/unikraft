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

/*++

Module Name:

    queue.c

Abstract:

    This file contains the queue entry points and callbacks.

Environment:

    User-mode Driver Framework 2

--*/

#include "driver.h"
#include "queue.tmh"

#include "FLC_LE.h"
#include "sgx_launch_public.h"
#include <bcrypt.h>


NTSTATUS
WinLeProxyUMDFQueueInitialize(
    _In_ WDFDEVICE Device
)
/*++

Routine Description:

     The I/O dispatch callbacks for the frameworks device object
     are configured in this function.

     A single default I/O Queue is configured for parallel request
     processing, and a driver context memory allocation is created
     to hold our structure QUEUE_CONTEXT.

Arguments:

    Device - Handle to a framework device object.

Return Value:

    VOID

--*/
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
        WdfIoQueueDispatchSequential
    );

    queueConfig.EvtIoDeviceControl = WinLeProxyUMDFEvtIoDeviceControl;
    queueConfig.EvtIoStop = WinLeProxyUMDFEvtIoStop;

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

static BOOLEAN calMRSigner(sgx_launch_token_request_t* buffer_in, sgx_sha256_hash_t value)
/*caculate the MRSigner*/
{
    BCRYPT_ALG_HANDLE       hAlg = NULL;
    BCRYPT_HASH_HANDLE      hHash = NULL;
    DWORD                   cbData = 0, cbHash = 0, cbHashObject = 0;
    PBYTE                   pbHashObject = NULL;
    BOOLEAN                 bRet = FALSE;
    NTSTATUS                status = STATUS_SUCCESS;

    if (buffer_in == NULL || value == NULL)
    {
        return FALSE;
    }

    //open an algorithm handle
    if (!NT_SUCCESS(status = BCryptOpenAlgorithmProvider(
        &hAlg,
        BCRYPT_SHA256_ALGORITHM,
        NULL,
        0)))
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_QUEUE,
            "**** Error 0x%x returned by BCryptOpenAlgorithmProvider\n", status);
        goto Cleanup;
    }

    //calculate the size of the buffer to hold the hash object
    if (!NT_SUCCESS(status = BCryptGetProperty(
        hAlg,
        BCRYPT_OBJECT_LENGTH,
        (PBYTE)&cbHashObject,
        sizeof(DWORD),
        &cbData,
        0)))
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_QUEUE,
            "**** Error 0x%x returned by BCryptGetProperty\n", status);
        goto Cleanup;
    }

    //allocate the hash object on the heap
    pbHashObject = (PBYTE)HeapAlloc(GetProcessHeap(), 0, cbHashObject);
    if (NULL == pbHashObject)
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_QUEUE,
            "**** memory allocation failed\n");
        goto Cleanup;
    }

    //calculate the length of the hash
    if (!NT_SUCCESS(status = BCryptGetProperty(
        hAlg,
        BCRYPT_HASH_LENGTH,
        (PBYTE)&cbHash,
        sizeof(DWORD),
        &cbData,
        0)))
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_QUEUE,
            "**** Error 0x%x returned by BCryptGetProperty\n", status);
        goto Cleanup;
    }

    //check the hash size
    if (cbHash != SGX_SHA256_HASH_SIZE)
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_QUEUE,
            "**** Hash Size is incorrect\n");
        goto Cleanup;
    }

    //create a hash
    if (!NT_SUCCESS(status = BCryptCreateHash(
        hAlg,
        &hHash,
        pbHashObject,
        cbHashObject,
        NULL,
        0,
        0)))
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_QUEUE,
            "**** Error 0x%x returned by BCryptCreateHash\n", status);
        goto Cleanup;
    }

    //hash some data
    if (!NT_SUCCESS(status = BCryptHashData(
        hHash,
        (PBYTE)buffer_in->css.key.modulus,
        SE_KEY_SIZE,
        0)))
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_QUEUE,
            "**** Error 0x%x returned by BCryptHashData\n", status);
        goto Cleanup;
    }

    //close the hash
    if (!NT_SUCCESS(status = BCryptFinishHash(
        hHash,
        value,
        cbHash,
        0)))
    {
        TraceEvents(TRACE_LEVEL_ERROR,
            TRACE_QUEUE,
            "**** Error 0x%x returned by BCryptFinishHash\n", status);
        goto Cleanup;
    }
    bRet = TRUE;
Cleanup:
    if (hAlg)
    {
        BCryptCloseAlgorithmProvider(hAlg, 0);
    }
    if (hHash)
    {
        BCryptDestroyHash(hHash);
    }
    if (pbHashObject)
    {
        HeapFree(GetProcessHeap(), 0, pbHashObject);
    }
    return bRet;
}

static NTSTATUS GetLaunchSupport(WDFDEVICE Device, sgx_get_launch_support_input_t* launch_support_Buffer_in, sgx_get_launch_support_output_t* launch_support_Buffer_out)
{
    NTSTATUS							status = STATUS_SUCCESS;
    WDF_MEMORY_DESCRIPTOR				inDescriptor;
    WDF_MEMORY_DESCRIPTOR				outDescriptor;
    WDFIOTARGET							wdfIoTarget;
    ULONG								ulBytesReturned = 0;
    WDF_OBJECT_ATTRIBUTES				wdfIoTargetAttributes;
    WDF_IO_TARGET_OPEN_PARAMS			wdfOpenParameters;


    if (!Device)
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "%!FUNC! Invalid Parameter\n");
        return STATUS_INVALID_PARAMETER;
    }

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
        &wdfIoTargetAttributes,
        DEVICE_CONTEXT);

    status = WdfIoTargetCreate(Device, &wdfIoTargetAttributes, &wdfIoTarget);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "%!FUNC! WdfIoTargetCreate failed with status = %!STATUS!", status);
        return status;
    }

    WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_FILE(&wdfOpenParameters, NULL);
    status = WdfIoTargetOpen(wdfIoTarget, &wdfOpenParameters);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "%!FUNC! WdfIoTargetOpen failed with status = %!STATUS!", status);
        WdfObjectDelete(wdfIoTarget);
        return status;
    }

    //send an IOCTL to the next lower driver to see if the PLE controls launch
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "%!FUNC! Send IOCTL IOCTL_SGX_GETLAUNCHSUPPORT to get LaunchSupport\n");
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&inDescriptor, (PVOID)launch_support_Buffer_in, sizeof(sgx_get_launch_support_input_t));
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&outDescriptor, (PVOID)launch_support_Buffer_out, sizeof(sgx_get_launch_support_output_t));

    status = WdfIoTargetSendIoctlSynchronously(wdfIoTarget, NULL, (ULONG)IOCTL_SGX_GETLAUNCHSUPPORT, &inDescriptor, &outDescriptor, NULL, (PULONG_PTR)&ulBytesReturned);
    if (NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "%!FUNC! IOCTL IOCTL_SGX_GETLAUNCHSUPPORT succeeded status = %!STATUS!", status);
    }
    else
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "%!FUNC! IOCTL IOCTL_SGX_GETLAUNCHSUPPORT Failed status = %!STATUS!", status);
    }
    WdfIoTargetClose(wdfIoTarget);
    WdfObjectDelete(wdfIoTarget);

    return status;

}

VOID
WinLeProxyUMDFEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
)
/*++

Routine Description:

    This event is invoked when the framework receives IRP_MJ_DEVICE_CONTROL request.

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
             I/O request.

    Request - Handle to a framework request object.

    OutputBufferLength - Size of the output buffer in bytes

    InputBufferLength - Size of the input buffer in bytes

    IoControlCode - I/O control code.

Return Value:

    VOID

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t buffer_size = 0;
    sgx_sha256_hash_t value = { 0 };
    PDEVICE_CONTEXT	deviceContext;
    WDFDEVICE		device;
    WDFFILEOBJECT	file_object;
    PUNICODE_STRING ioctl_interface_name = NULL;     //points to the name of the interface that the IOCTL was sent to
    UNICODE_STRING  trimmed_interface_name;          //interface name after it is trimmed
    BOOLEAN			bProcessIOCTL = FALSE;

    TraceEvents(TRACE_LEVEL_INFORMATION,
                TRACE_QUEUE,
                "%!FUNC! Queue 0x%p, Request 0x%p OutputBufferLength %d InputBufferLength %d IoControlCode %d",
                Queue, Request, (int)OutputBufferLength, (int)InputBufferLength, IoControlCode);

    DECLARE_CONST_UNICODE_STRING(sgxLaunchTokenInterfaceName, SGX_LAUNCH_TOKEN_INTERFACE_NAME);
    device = WdfIoQueueGetDevice(Queue);
    deviceContext = DeviceGetContext(device);

    //We need to check that this is our interface.  If not, then just pass the IOCTL on to the next driver
    //  First get the file object
    file_object = WdfRequestGetFileObject(Request);
    if (!file_object)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "%!FUNC! Failed to get File Object!");
        //we should still pass this IOCTL on since we don't know that it is ours
    }
    else
    {
        ioctl_interface_name = WdfFileObjectGetFileName(file_object);
        if (ioctl_interface_name == NULL)
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "%!FUNC! Failed to get Interface Name!");
        }
        else
        {
            if (ioctl_interface_name->Length < 2)
            {
                TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "%!FUNC! Invalid Interface Name\n");
            }
            else
            {
                //remove the first '\' in interface_name
                RtlInitUnicodeString(&trimmed_interface_name, ioctl_interface_name->Buffer + 1);

                if (RtlEqualUnicodeString(&trimmed_interface_name, &sgxLaunchTokenInterfaceName, TRUE) == FALSE)
                {
                    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "%!FUNC! IOCTL do not use correct driver interface\n");
                }
                else
                {
                    bProcessIOCTL = TRUE;
                }
            }
        }
    }

    if (bProcessIOCTL)
    {
        if (IoControlCode == IOCTL_SGX_GETTOKEN)
        {
            sgx_launch_request_t req = { 0 };
            WDFMEMORY memory = NULL;


            //if the PLE is not loaded, try to load it.
            if (entry == NULL)
                entry = start_launch_enclave();

            if (entry == NULL)
            {
                TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "Failed to load PLE");
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }

            status = WdfRequestRetrieveInputMemory(Request, &memory);
            if (status != STATUS_SUCCESS)
            {
                goto end;
            }

            sgx_launch_token_request_t* buffer_in = (sgx_launch_token_request_t*)WdfMemoryGetBuffer(memory, &buffer_size);
            if (buffer_in == NULL || buffer_size < sizeof(sgx_launch_token_request_t) || buffer_in->version != 0)
            {
                buffer_size = 0;
                goto end;
            }

            status = WdfRequestRetrieveOutputMemory(Request, &memory);
            if (status != STATUS_SUCCESS)
            {
                buffer_size = 0;
                goto end;
            }

            sgx_le_output_t* buffer_out = (sgx_le_output_t*)WdfMemoryGetBuffer(memory, &buffer_size);
            if (buffer_out == NULL || buffer_size < sizeof(sgx_le_output_t))
            {
                buffer_size = 0;
                goto end;
            }

            if (calMRSigner(buffer_in, value) == FALSE)
            {
                buffer_size = 0;
                goto end;
            }

            req.attributes = buffer_in->secs_attr.flags;
            req.xfrm = buffer_in->secs_attr.xfrm;
            memcpy_s(req.mrenclave, 32, &buffer_in->css.body.enclave_hash, sizeof(sgx_measurement_t));
            memcpy_s(req.mrsigner, 32, &value, sizeof(sgx_sha256_hash_t));

            try
            {
                sgx_get_token(&req, entry);
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                TraceEvents(TRACE_LEVEL_ERROR,
                    TRACE_QUEUE,
                    "sgx_get_token crashed");
                goto end;
            }

            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "sgx_get_token finished");

            errno_t err = memcpy_s(buffer_out, buffer_size, &req.output, sizeof(sgx_le_output_t));
            if (err)
                goto end;

            status = STATUS_SUCCESS;
        }
        else if(IoControlCode == IOCTL_SGX_GETLAUNCHSUPPORT)
        {
            WDFMEMORY memory = NULL;
            sgx_get_launch_support_output_t launch_support_output_Buffer = { 0 };


            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "IOCTL_SGX_GETLAUNCHSUPPORT");

            status = WdfRequestRetrieveInputMemory(Request, &memory);
            if (status != STATUS_SUCCESS)
            {
                buffer_size = 0;
                goto end;
            }

            sgx_get_launch_support_input_t* buffer_in = (sgx_get_launch_support_input_t*)WdfMemoryGetBuffer(memory, &buffer_size);
            if (buffer_in == NULL || buffer_size < sizeof(sgx_get_launch_support_input_t) || buffer_in->version != 0)
            {
                buffer_size = 0;
                goto end;
            }

            status = WdfRequestRetrieveOutputMemory(Request, &memory);
            if (status != STATUS_SUCCESS)
            {
                buffer_size = 0;
                goto end;
            }

            sgx_get_launch_support_output_t* buffer_out = (sgx_get_launch_support_output_t*)WdfMemoryGetBuffer(memory, &buffer_size);
            if (buffer_out == NULL || buffer_size < sizeof(sgx_get_launch_support_output_t))
            {
                buffer_size = 0;
                goto end;
            }

            status = GetLaunchSupport(device, buffer_in, &launch_support_output_Buffer);
            if (status != STATUS_SUCCESS)
            {
                buffer_size = 0;
                goto end;
            }

            errno_t err = memcpy_s(buffer_out, buffer_size, &launch_support_output_Buffer, sizeof(sgx_get_launch_support_output_t));

            if (err)
                goto end;

            buffer_size = sizeof(sgx_get_launch_support_output_t);
            status = STATUS_SUCCESS;


            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "%!FUNC! IOCTL IOCTL_SGX_GETLAUNCHSUPPORT returning STATUS_SUCCESS\n");

            WdfRequestCompleteWithInformation(Request, status, buffer_size);
            return;
        }
        else
        {
            status = STATUS_INVALID_DEVICE_REQUEST;
            WdfRequestComplete(Request, status);
            return;
        }
      

end:
    WdfRequestCompleteWithInformation(Request, status, buffer_size);
    }
    else
    {
        WDFIOTARGET ioTargetHandle = NULL;
        WDF_REQUEST_SEND_OPTIONS options;

        device = WdfIoQueueGetDevice(Queue);
        ioTargetHandle = WdfDeviceGetIoTarget(device);

        WdfRequestFormatRequestUsingCurrentType(Request);

        WDF_REQUEST_SEND_OPTIONS_INIT(&options,
            WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET);
        WdfRequestSend(Request, ioTargetHandle, &options);

        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "%!FUNC! forward the request to the KMDF driver");
        return;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "%!FUNC! Exit");
    return;
}

VOID
WinLeProxyUMDFEvtIoStop(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ ULONG ActionFlags
)
/*++

Routine Description:

    This event is invoked for a power-managed queue before the device leaves the working state (D0).

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
             I/O request.

    Request - Handle to a framework request object.

    ActionFlags - A bitwise OR of one or more WDF_REQUEST_STOP_ACTION_FLAGS-typed flags
                  that identify the reason that the callback function is being called
                  and whether the request is cancelable.

Return Value:

    VOID

--*/
{
    TraceEvents(TRACE_LEVEL_INFORMATION,
                TRACE_QUEUE,
                "%!FUNC! Queue 0x%p, Request 0x%p ActionFlags %d",
                Queue, Request, ActionFlags);

    return;
}

