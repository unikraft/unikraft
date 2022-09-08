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

#include "Power.h"
#include "Power.tmh"
#include "Utility.h"
#include "sgx_lc_msr_public.h"
#include "Key.h"

//static BOOLEAN OwnFLC = FALSE;
sgx_get_launch_support_output_t launch_support_info = { 0 };
SGX_PUBKEYHASH  legacypubKeyHash = { 0 };

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FLCMSREvtDeviceD0Exit)
#endif // ALLOC_PRAGMA

ULONG *PROCESSOR_MSR_FLAG = NULL;
PKDPC pkdpc = NULL;
PKTHREAD gFLCNotifyRegistryChangeThreadObject = NULL;
BOOLEAN gFLCNotifyRegistryChangeThreadStatus = FALSE;
HANDLE gRegKeyHandle = NULL;

KDEFERRED_ROUTINE WriteMsrRoutine;
KSTART_ROUTINE FLCMSRNotifyRegistryChangeRoutine;

void WriteMsrRoutine(
    KDPC *Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument2);
    BOOLEAN UsePLEOptIn = !!(SystemArgument1);
    ULONG ret = 0;
    __int64 feature_control = 0;

    try
    {
        feature_control = __readmsr(MSR_IA32_FEATURE_CONTROL);

        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_UTILITY, "MSR_IA32_FEATURE_CONTROL %llx", feature_control);

        if (feature_control & (1 << 17))
        {
            if (UsePLEOptIn)
            {
                TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_UTILITY, "Writing MSRs %llx %llx %llx %llx",
                    MSR_IA32_SGX_LE_PUBKEYHASH_VALUE_0, MSR_IA32_SGX_LE_PUBKEYHASH_VALUE_1, MSR_IA32_SGX_LE_PUBKEYHASH_VALUE_2, MSR_IA32_SGX_LE_PUBKEYHASH_VALUE_3);
                __writemsr(MSR_IA32_SGX_LE_PUBKEYHASH_0, MSR_IA32_SGX_LE_PUBKEYHASH_VALUE_0);
                __writemsr(MSR_IA32_SGX_LE_PUBKEYHASH_1, MSR_IA32_SGX_LE_PUBKEYHASH_VALUE_1);
                __writemsr(MSR_IA32_SGX_LE_PUBKEYHASH_2, MSR_IA32_SGX_LE_PUBKEYHASH_VALUE_2);
                __writemsr(MSR_IA32_SGX_LE_PUBKEYHASH_3, MSR_IA32_SGX_LE_PUBKEYHASH_VALUE_3);
            }
            else
            {
                TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_UTILITY, "Writing legacypubKeyHash %llx %llx %llx %llx",
                    legacypubKeyHash.pubKeyHash_Value_0, legacypubKeyHash.pubKeyHash_Value_1, legacypubKeyHash.pubKeyHash_Value_2, legacypubKeyHash.pubKeyHash_Value_3);
                __writemsr(MSR_IA32_SGX_LE_PUBKEYHASH_0, legacypubKeyHash.pubKeyHash_Value_0);
                __writemsr(MSR_IA32_SGX_LE_PUBKEYHASH_1, legacypubKeyHash.pubKeyHash_Value_1);
                __writemsr(MSR_IA32_SGX_LE_PUBKEYHASH_2, legacypubKeyHash.pubKeyHash_Value_2);
                __writemsr(MSR_IA32_SGX_LE_PUBKEYHASH_3, legacypubKeyHash.pubKeyHash_Value_3);
            }
            ret = 1;
        }
        else
        {
            ret = 2;
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = 3;
    }

    ULONG current_processor = KeGetCurrentProcessorNumber();
    if (PROCESSOR_MSR_FLAG != NULL)
    {
        PROCESSOR_MSR_FLAG[current_processor] = ret;
    }
    return;
}

void ReadMsr()
{
    __int64 feature_control = 0;

    try
    {
        feature_control = __readmsr(MSR_IA32_FEATURE_CONTROL);

        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_UTILITY, "MSR_IA32_FEATURE_CONTROL %llx", feature_control);

        if (feature_control & (1 << 17))
        {
            legacypubKeyHash.pubKeyHash_Value_0 = __readmsr(MSR_IA32_SGX_LE_PUBKEYHASH_0);
            legacypubKeyHash.pubKeyHash_Value_1 = __readmsr(MSR_IA32_SGX_LE_PUBKEYHASH_1);
            legacypubKeyHash.pubKeyHash_Value_2 = __readmsr(MSR_IA32_SGX_LE_PUBKEYHASH_2);
            legacypubKeyHash.pubKeyHash_Value_3 = __readmsr(MSR_IA32_SGX_LE_PUBKEYHASH_3);
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_UTILITY, "read msr The Key is %llx %llx %llx %llx",
        legacypubKeyHash.pubKeyHash_Value_0,
        legacypubKeyHash.pubKeyHash_Value_1,
        legacypubKeyHash.pubKeyHash_Value_2,
        legacypubKeyHash.pubKeyHash_Value_3);
    return;
}

static BOOLEAN is_PLE_OPT_IN()
{
    ULONG ulPLEOptIn = 0;

    //Get PLE Opt-In from the Registry
    WDFKEY key;
    NTSTATUS status = WdfDriverOpenParametersRegistryKey(WdfGetDriver(), KEY_READ, WDF_NO_OBJECT_ATTRIBUTES, &key);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_POWER, "%!FUNC! WdfDriverOpenParametersRegistryKey failed %!STATUS!", status);
    }
    else
    {
        DECLARE_CONST_UNICODE_STRING(valueName, SGX_PLE_REGISTRY_OPT_IN_REGISTRY);
        status = WdfRegistryQueryULong(key, &valueName, &ulPLEOptIn);
        if (!NT_SUCCESS(status))
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_POWER, "%!FUNC! WdfRegistryQueryULong failed %!STATUS!", status);
        }
        WdfRegistryClose(key);
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_POWER, "PLE_OPT_IN:0x%x", ulPLEOptIn);

    if (ulPLEOptIn == 1)
        return TRUE;
    else
        return FALSE;
}

static BOOLEAN FLCWriteMSRs(BOOLEAN UsePLEOptIn)
{
    ULONG maximumProcessor = 0;
    ULONG i = 0;
    ULONG count = 0;
    BOOLEAN ret = FALSE;
    PKDPC tmp_pkdc = NULL;

    maximumProcessor = KeQueryActiveProcessorCount(NULL);
    if (PROCESSOR_MSR_FLAG == NULL)
    {
        PROCESSOR_MSR_FLAG = (ULONG*)ExAllocatePoolWithTag(NonPagedPool, maximumProcessor * sizeof(ULONG), 'flag');
        if (PROCESSOR_MSR_FLAG == NULL)
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_POWER, " Insufficient memory");
            return FALSE;
        }
    }

    if (pkdpc == NULL)
    {
        pkdpc = (PKDPC)ExAllocatePoolWithTag(NonPagedPool, maximumProcessor * sizeof(KDPC), 'kdpc');
        if (pkdpc == NULL)
        {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_POWER, " Insufficient memory");
            return FALSE;
        }
        tmp_pkdc = pkdpc;
        for (i = 0; i < maximumProcessor; i++, tmp_pkdc++)
        {
            KeInitializeThreadedDpc(tmp_pkdc, WriteMsrRoutine, NULL);
        }
    }

    tmp_pkdc = pkdpc;
    for (i = 0; i < maximumProcessor; i++, tmp_pkdc++)
    {
        PROCESSOR_MSR_FLAG[i] = 0;
        KeSetTargetProcessorDpc(tmp_pkdc, (CCHAR)i);
        KeInsertQueueDpc(tmp_pkdc, (PVOID)UsePLEOptIn, NULL);
    }

    while (1)
    {
        count = 0;
        for (i = 0; i < maximumProcessor; i++)
        {
            if (PROCESSOR_MSR_FLAG[i] != 0)
            {
                count++;
            }
            else
            {
                break;
            }
        }
        if (count == maximumProcessor)
        {
            break;
        }
    }
    ret = TRUE;
    for (i = 0; i < maximumProcessor; i++)
    {
        if (PROCESSOR_MSR_FLAG[i] == 3)
        {
            ret = FALSE;
            break;
        }
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_POWER, "Write MSRs finished %u", ret);
    return ret;
}

void watch_registry(HANDLE regh) {
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;
    ULONG ret = 0;
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_POWER, "%!FUNC! Entry");

    status = ZwNotifyChangeKey(regh, NULL, NULL, (PVOID)DelayedWorkQueue, &iosb, REG_NOTIFY_CHANGE_LAST_SET, TRUE, NULL, 0, FALSE);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_POWER, "%!FUNC! ZwNotifyChangeKey failed %!STATUS!", status);
        return;
    }

    if (is_PLE_OPT_IN())
    {
        launch_support_info.configurationFlags |= SGX_PLE_REGISTRY_OPT_IN;
        ret = FLCWriteMSRs(TRUE);
        if (ret == TRUE)
        {
            launch_support_info.configurationFlags |= SGX_LCP_PLATFORM_SUPPORT;
            launch_support_info.pubKeyHash.pubKeyHash_Value_0 = MSR_IA32_SGX_LE_PUBKEYHASH_VALUE_0;
            launch_support_info.pubKeyHash.pubKeyHash_Value_1 = MSR_IA32_SGX_LE_PUBKEYHASH_VALUE_1;
            launch_support_info.pubKeyHash.pubKeyHash_Value_2 = MSR_IA32_SGX_LE_PUBKEYHASH_VALUE_2;
            launch_support_info.pubKeyHash.pubKeyHash_Value_3 = MSR_IA32_SGX_LE_PUBKEYHASH_VALUE_3;
        }
    }
    else
    {
        launch_support_info.configurationFlags &= ~SGX_PLE_REGISTRY_OPT_IN;
        ret = FLCWriteMSRs(FALSE);
        if (ret == TRUE)
        {
            launch_support_info.configurationFlags &= ~SGX_LCP_PLATFORM_SUPPORT;
            launch_support_info.pubKeyHash.pubKeyHash_Value_0 = legacypubKeyHash.pubKeyHash_Value_0;
            launch_support_info.pubKeyHash.pubKeyHash_Value_1 = legacypubKeyHash.pubKeyHash_Value_1;
            launch_support_info.pubKeyHash.pubKeyHash_Value_2 = legacypubKeyHash.pubKeyHash_Value_2;
            launch_support_info.pubKeyHash.pubKeyHash_Value_3 = legacypubKeyHash.pubKeyHash_Value_3;
        }
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_POWER, "%!FUNC! exit");
    return;
}



void FLCMSRNotifyRegistryChangeRoutine(PVOID StartContext)
{
    NTSTATUS status;
    WDFKEY key;
    UNREFERENCED_PARAMETER(StartContext);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_POWER, "%!FUNC! Entry");

    status = WdfDriverOpenParametersRegistryKey(WdfGetDriver(), KEY_ALL_ACCESS, WDF_NO_OBJECT_ATTRIBUTES, &key);
    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_POWER, "%!FUNC! WdfDriverOpenParametersRegistryKey failed %!STATUS!", status);
        PsTerminateSystemThread(status);
        return;
    }

    gRegKeyHandle = WdfRegistryWdmGetHandle(key);
   
    while (gFLCNotifyRegistryChangeThreadStatus == TRUE)
    {
        watch_registry(gRegKeyHandle);
    }
  
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_POWER, "%!FUNC! exit");
    PsTerminateSystemThread(STATUS_SUCCESS);
    return;
}

static BOOLEAN FLCMSRNotifyRegistryChange()
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE thread_handle;
    gFLCNotifyRegistryChangeThreadStatus = TRUE;

    status = PsCreateSystemThread(&thread_handle,
        THREAD_ALL_ACCESS,
        NULL,
        NULL,
        NULL,
        FLCMSRNotifyRegistryChangeRoutine,
        NULL);

    if (!NT_SUCCESS(status))
    {
        gFLCNotifyRegistryChangeThreadStatus = FALSE;
        return FALSE;
    }

    status = ObReferenceObjectByHandle(
        thread_handle,
        THREAD_ALL_ACCESS,
        NULL,
        KernelMode,
        &gFLCNotifyRegistryChangeThreadObject,
        NULL);

    if (!NT_SUCCESS(status))
    {
        gFLCNotifyRegistryChangeThreadStatus = FALSE;
        return FALSE;
    }
    
    ZwClose(thread_handle);
    return TRUE;
}


NTSTATUS
FLCMSREvtDeviceD0Entry(
    IN WDFDEVICE                Device,
    IN WDF_POWER_DEVICE_STATE   RecentPowerState
)
{
    BOOLEAN ret = 0;

    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(RecentPowerState);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_POWER, "%!FUNC! Entry");

    if (is_OS_support_FLC())
        launch_support_info.configurationFlags = SGX_LCP_OS_PERMISSION;
    else
        return STATUS_SUCCESS;

    //clear the platform support bit, set the bit if and only if successfuly update MSR
    launch_support_info.configurationFlags &= ~SGX_LCP_PLATFORM_SUPPORT;

    if (is_HW_support_FLC())
    {
        ReadMsr();
    }

    if (is_PLE_OPT_IN())
    {
        launch_support_info.configurationFlags |= SGX_PLE_REGISTRY_OPT_IN;

        if (is_HW_support_FLC())
        {
            ret = FLCWriteMSRs(TRUE);

            if (ret == TRUE)
            {
                launch_support_info.configurationFlags |= SGX_LCP_PLATFORM_SUPPORT;
                launch_support_info.pubKeyHash.pubKeyHash_Value_0 = MSR_IA32_SGX_LE_PUBKEYHASH_VALUE_0;
                launch_support_info.pubKeyHash.pubKeyHash_Value_1 = MSR_IA32_SGX_LE_PUBKEYHASH_VALUE_1;
                launch_support_info.pubKeyHash.pubKeyHash_Value_2 = MSR_IA32_SGX_LE_PUBKEYHASH_VALUE_2;
                launch_support_info.pubKeyHash.pubKeyHash_Value_3 = MSR_IA32_SGX_LE_PUBKEYHASH_VALUE_3;
            }
        }
    }
    
    if (is_HW_support_FLC())
    {
        FLCMSRNotifyRegistryChange();
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_POWER, "%!FUNC! Exit %x", launch_support_info.configurationFlags);

    //always return success because we need to support IOCTL_SGX_GETLAUNCHSUPPORT
    return STATUS_SUCCESS;
}

NTSTATUS
FLCMSREvtDeviceD0Exit(
    IN WDFDEVICE                Device,
    IN WDF_POWER_DEVICE_STATE   PowerState
)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(PowerState);

    if (gFLCNotifyRegistryChangeThreadStatus == TRUE)
    {
        gFLCNotifyRegistryChangeThreadStatus = FALSE;
        if (gRegKeyHandle != NULL)
        {
            ZwClose(gRegKeyHandle);
            gRegKeyHandle = NULL;
        }

        KeWaitForSingleObject(
            gFLCNotifyRegistryChangeThreadObject,
            Executive,
            KernelMode,
            FALSE,
            NULL
        );
        ObDereferenceObject(gFLCNotifyRegistryChangeThreadObject);
    }

    if (is_HW_support_FLC())
    {
        FLCWriteMSRs(FALSE);
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_POWER, "Write legacy LE mrsigner when exit");
    }

    if (PROCESSOR_MSR_FLAG)
    {
        ExFreePoolWithTag(PROCESSOR_MSR_FLAG, 'flag');
        PROCESSOR_MSR_FLAG = NULL;
    }
    if (pkdpc)
    {
        ExFreePoolWithTag(pkdpc, 'kdpc');
        pkdpc = NULL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
FLCMSREvtDevicePrepareHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourceList,
    _In_ WDFCMRESLIST ResourceListTranslated
)
/*++

Routine Description:

In this callback, the driver does whatever is necessary to make the
hardware ready to use.  In the case of a USB device, this involves
reading and selecting descriptors.

Arguments:

Device - handle to a device

Return Value:

NT status value

--*/
{
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(ResourceList);
    UNREFERENCED_PARAMETER(ResourceListTranslated);

    return STATUS_SUCCESS;
}