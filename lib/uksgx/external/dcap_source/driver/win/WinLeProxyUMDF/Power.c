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

#include "wudfwdm.h"
#include "windows.h"

#include "power.tmh"

#define LE_BLOB_LEN 57344

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LEProxyEvtDeviceD0Exit)
#pragma alloc_text(PAGE, LEProxyEvtDeviceArmWakeFromS0)
#pragma alloc_text(PAGE, LEProxyEvtDeviceArmWakeFromSx)
#pragma alloc_text(PAGE, LEProxyEvtDeviceWakeFromS0Triggered)
#pragma alloc_text(PAGE, DbgDevicePowerString)
#endif // ALLOC_PRAGMA

void *entry = NULL;

NTSTATUS
LEProxyEvtDeviceD0Entry(
    IN WDFDEVICE                Device,
    IN WDF_POWER_DEVICE_STATE   RecentPowerState
)
/*++
Routine Description:

EvtDeviceD0Entry event is called to program the device to goto
D0, which is the working state. The framework calls the driver's
EvtDeviceD0Entry callback when the Power manager sends an
IRP_MN_SET_POWER-DevicePower request to the driver stack. The Power manager
sends this request when the power policy manager of this device stack
(probaby the FDO) requests a change in D-state by calling PoRequestPowerIrp.

This function is not marked pageable because this function is in the
device power up path. When a function is marked pagable and the code
section is paged out, it will generate a page fault which could impact
the fast resume behavior because the client driver will have to wait
until the system drivers can service this page fault.

Arguments:

Device - handle to a framework device object.

RecentPowerState - WDF_POWER_DEVICE_STATE-typed enumerator that identifies the
device power state that the device was in before this transition
to D0.

Return Value:

NTSTATUS    - A failure here will indicate a fatal error in the driver.
The Framework will attempt to tear down the stack.

--*/
{
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(RecentPowerState);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_POWER, "%!FUNC! Entry");

    entry = NULL;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_POWER, "%!FUNC! Exit");
    return STATUS_SUCCESS;
}

NTSTATUS
LEProxyEvtDeviceD0Exit(
    IN WDFDEVICE                Device,
    IN WDF_POWER_DEVICE_STATE   PowerState
)
/*++
Routine Description:

EvtDeviceD0Entry event is called to program the device to goto
D1, D2 or D3, which are the low-power states. The framework calls the
driver's EvtDeviceD0Exit callback when the Power manager sends an
IRP_MN_SET_POWER-DevicePower request to the driver stack. The Power manager
sends this request when the power policy manager of this device stack
(probaby the FDO) requests a change in D-state by calling PoRequestPowerIrp.

Arguments:

Device - handle to a framework device object.

DeviceState - WDF_POWER_DEVICE_STATE-typed enumerator that identifies the
device power state that the power policy owner (probably the
FDO) has decided is appropriate.

Return Value:

NTSTATUS    - A failure here will indicate a fatal error in the driver.
The Framework will attempt to tear down the stack.
--*/
{
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(PowerState);

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_POWER, "%!FUNC! Entry");

    if (entry)
    {
        VirtualFreeEx(
            GetCurrentProcess(),
            entry,
            0,
            MEM_RELEASE);

        entry = NULL;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_POWER, "%!FUNC! Exit");
    return STATUS_SUCCESS;
}




