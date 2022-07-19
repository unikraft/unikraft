/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Cristian Vijelie <cristianvijelie@gmail.com>
 *
 * Copyright (c) 2021, University POLITEHNICA of Bucharest. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef __PLAT_CMN_X86_MADT_H__
#define __PLAT_CMN_X86_MADT_H__

#include <uk/arch/types.h>
#include <x86/acpi/sdt.h>
#include <uk/essentials.h>

struct MADT {
	struct ACPISDTHeader h;
	__u32 LocalAPICAddress;
	__u32 Flags;
	__u8 Entries[];
} __packed;

#define MADT_FLAGS_PCAT_COMPAT			0x01

struct MADTEntryHeader {
	__u8 Type;
	__u8 Length;
} __packed;

#define MADT_TYPE_LAPIC				0x00
#define MADT_TYPE_IO_APIC			0x01
#define MADT_TYPE_INT_SRC_OVERRIDE		0x02
#define MADT_TYPE_NMI_SOURCE			0x03
#define MADT_TYPE_LAPIC_NMI			0x04
#define MADT_TYPE_LAPIC_ADDRESS_OVERRIDE	0x05
#define MADT_TYPE_IO_SAPIC			0x06
#define MADT_TYPE_LSAPIC			0x07
#define MADT_TYPE_PLATFORM_INT_SOURCES		0x08
#define MADT_TYPE_LX2APIC			0x09
#define MADT_TYPE_LX2APIC_NMI			0x0a
#define MADT_TYPE_GICC				0x0b
#define MADT_TYPE_GICD				0x0c
#define MADT_TYPE_GIC_MSI			0x0d
#define MADT_TYPE_GICR				0x0e
#define MADT_TYPE_GIC_ITS			0x0f
#define MADT_TYPE_MP_WAKEUP			0x10

/*
 * The following structures are declared according to the ACPI
 * specification version 6.3.
 *
 * TODO: This header includes structures that are not related to x86. However,
 * we move the header when integrating other architectures.
 */

/* Processor Local APIC Structure */
struct MADTType0Entry {
	struct MADTEntryHeader eh;
	__u8 ACPIProcessorUID;
	__u8 APICID;
	__u32 Flags;
} __packed;

#define MADT_T0_FLAGS_ENABLED			0x01
#define MADT_T0_FLAGS_ONLINE_CAPABLE		0x02

/* I/O APIC Structure */
struct MADTType1Entry {
	struct MADTEntryHeader eh;
	__u8 IOAPICID;
	__u8 Reserved;
	__u32 IOAPICAddress;
	__u32 GlobalSystemInterruptBase;
} __packed;

/* Interrupt Source Override Structure */
struct MADTType2Entry {
	struct MADTEntryHeader eh;
	__u8 BusSource;
	__u8 IRQSource;
	__u32 GlobalSystemInterrupt;
	__u16 Flags;
} __packed;

/* Non-Maskable Interrupt (NMI) Source Structure */
struct MADTType3Entry {
	struct MADTEntryHeader eh;
	__u16 Flags;
	__u32 GlobalSystemInterrupt;
} __packed;

/* Local APIC NMI Structure */
struct MADTType4Entry {
	struct MADTEntryHeader eh;
	__u8 ACPIProcessorID;
	__u16 Flags;
	__u8 LINT;
} __packed;

/* Local APIC Address Override Structure */
struct MADTType5Entry {
	struct MADTEntryHeader eh;
	__u16 Reserved;
	__u64 LAPICOverride;
} __packed;

/* I/O SAPIC Structure */
struct MADTType6Entry {
	struct MADTEntryHeader eh;
	__u8 IOAPICID;
	__u8 Reserved;
	__u32 GlobalSystemInterruptBase;
	__u64 IOSAPICAddress;
} __packed;

/* Local SAPIC Structure */
struct MADTType7Entry {
	struct MADTEntryHeader eh;
	__u8 ACPIProcessorID;
	__u8 LocalSAPICID;
	__u8 LocalSAPICEID;
	__u8 Reserved[3];
	__u32 Flags;
	__u32 ACPIProcessorUIDValue;
	__u32 ACPIProcessorUIDString[];
} __packed;

/* Platform Interrupt Source Structure */
struct MADTType8Entry {
	struct MADTEntryHeader eh;
	__u16 Flags;
	__u8 InterruptType;
	__u8 ProcessorID;
	__u8 ProcessorEID;
	__u8 IOSAPICVector;
	__u32 GlobalSystemInterrupt;
	__u32 PlatformInterruptSourceFlags;
} __packed;

/* Processor Local x2APIC Structure */
struct MADTType9Entry {
	struct MADTEntryHeader eh;
	__u16 Reserved;
	__u32 X2APICID;
	__u32 Flags;
	__u32 ACPIProcessorUID;
} __packed;

#define MADT_T9_FLAGS_ENABLED			0x01
#define MADT_T9_FLAGS_ONLINE_CAPABLE		0x02

/* Local x2APIC NMI Structure */
struct MADTTypeAEntry {
	struct MADTEntryHeader eh;
	__u16 Flags;
	__u32 ACPIProcessorUID;
	__u8 Localx2APICLINTNr;
	__u8 Reserved[3];
} __packed;

/* GIC CPU Interface (GICC) Structure */
struct MADTTypeBEntry {
	struct MADTEntryHeader eh;
	__u16 Reserved;
	__u32 CPUInterfaceNumber;
	__u32 ACPIProcessorUID;
	__u32 Flags;
	__u32 ParkingProtocolVersion;
	__u32 PerformanceInterruptGSIV;
	__u64 ParkedAddress;
	__u64 PhysicalBaseAddress;
	__u64 GICV;
	__u64 GICH;
	__u32 VGICMaintenanceInterrupt;
	__u64 GICRBaseAddress;
	__u64 MPIDR;
	__u8 ProcessorPowerEfficiencyClass;
	__u8 Reserved2;
	__u16 SPEOverflowInterrupt;
} __packed;

/* GIC Distributor (GICD) Structure */
struct MADTTypeCEntry {
	struct MADTEntryHeader eh;
	__u16 Reserved;
	__u32 GICID;
	__u64 PhysicalBaseAddress;
	__u32 SystemVectorBase;
	__u8 GICVersion;
	__u8 Reserved2[3];
} __packed;

/* GIC MSI Frame Structure */
struct MADTTypeDEntry {
	struct MADTEntryHeader eh;
	__u16 Reserved;
	__u32 GICMSIFrameID;
	__u64 PhysicalBaseAddress;
	__u32 Flags;
	__u16 SPICount;
	__u16 SPIBase;
} __packed;

/* GIC Redistributor (GICR) Structure */
struct MADTTypeEEntry {
	struct MADTEntryHeader eh;
	__u16 Reserved;
	__u64 DiscoveryRangeBaseAddress;
	__u64 DiscoveryRangeLength;
} __packed;

/* GIC Interrupt Translation Service (ITS) Structure */
struct MADTTypeFEntry {
	struct MADTEntryHeader eh;
	__u16 Reserved;
	__u32 GICITSID;
	__u64 PhysicalBaseAddress;
	__u32 Reserved2;
} __packed;

/* Multiprocessor Wakeup Structure */
struct MADTType10Entry {
	struct MADTEntryHeader eh;
	__u16 MailBoxVersion;
	__u32 Reserved;
	__u64 MailBoxAddress;
} __packed;

/**
 * Return the Multiple APIC Descriptor Table (MADT). ACPI needs to be
 * initialized first.
 *
 * @return Pointer to MADT.
 */
struct MADT *acpi_get_madt(void);

#endif /* __PLAT_CMN_X86_MADT_H__ */
