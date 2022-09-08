/*=============================================================================

  Library: CppMicroServices

  Copyright (c) The CppMicroServices developers. See the COPYRIGHT
  file at the top-level directory of this distribution and at
  https://github.com/CppMicroServices/CppMicroServices/COPYRIGHT .

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

=============================================================================*/

// The contents of this file was created from Microsoft's publicly
// available PE/COFF format document:
//
// Microsoft Portable Executable and Common Object File Format Specification
// Revision 8.3 – February 6, 2013
//
// http://msdn.microsoft.com/en-us/windows/hardware/gg463119.aspx

#ifndef CPPMICROSERVICES_PE_H
#define CPPMICROSERVICES_PE_H

#include <cstdint>

#include "cppmicroservices/GlobalConfig.h"

namespace cppmicroservices {

static const uint32_t PE_SIGNATURE = 0x00004550; // { 'P', 'E', '\0', '\0' }
static const uint16_t DOS_SIGNATURE = 0x5A4D; // { 'M', 'Z' }

static const uint16_t OPTIONAL_PE32_MAGIC = 0x10b; // PE32 format
static const uint16_t OPTIONAL_PE64_MAGIC = 0x20b; // PE32+ format

enum {
  NameSize = 8,
  NumberOfDataDirectories = 16
};

struct DOSHeader
{
  uint16_t Magic;
  uint16_t Useduint8_tsInTheLastPage;
  uint16_t FileSizeInPages;
  uint16_t NumberOfRelocationItems;
  uint16_t HeaderSizeInParagraphs;
  uint16_t MinimumExtraParagraphs;
  uint16_t MaximumExtraParagraphs;
  uint16_t InitialRelativeSS;
  uint16_t InitialSP;
  uint16_t Checksum;
  uint16_t InitialIP;
  uint16_t InitialRelativeCS;
  uint16_t AddressOfRelocationTable;
  uint16_t OverlayNumber;
  uint16_t Reserved[4];
  uint16_t OEMid;
  uint16_t OEMinfo;
  uint16_t Reserved2[10];
  uint32_t AddressOfNewExeHeader;
};

struct COFFHeader
{
  uint16_t Machine;
  uint16_t NumberOfSections;
  uint32_t TimeDateStamp;
  uint32_t PointerToSymbolTable;
  uint32_t NumberOfSymbols;
  uint16_t SizeOfOptionalHeader;
  uint16_t Characteristics;
};

enum Characteristics {
  IMAGE_FILE_RELOCS_STRIPPED         = 0x0001,
  IMAGE_FILE_EXECUTABLE_IMAGE        = 0x0002,
  IMAGE_FILE_LINE_NUMS_STRIPPED      = 0x0004,
  IMAGE_FILE_LOCAL_SYMS_STRIPPED     = 0x0008,
  IMAGE_FILE_AGGRESIVE_WS_TRIM       = 0x0010,
  IMAGE_FILE_LARGE_ADDRESS_AWARE     = 0x0020,
  IMAGE_FILE_uint8_tS_REVERSED_LO    = 0x0080,
  IMAGE_FILE_32BIT_MACHINE           = 0x0100,
  IMAGE_FILE_DEBUG_STRIPPED          = 0x0200,
  IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP = 0x0400,
  IMAGE_FILE_NET_RUN_FROM_SWAP       = 0x0800,
  IMAGE_FILE_SYSTEM                  = 0x1000,
  IMAGE_FILE_DLL                     = 0x2000,
  IMAGE_FILE_UP_SYSTEM_ONLY          = 0x4000,
  IMAGE_FILE_uint8_tS_REVERSED_HI    = 0x8000
};

struct DataDirectory
{
  uint32_t VirtualAddress;
  uint32_t Size;
};

struct OptionalHeader32
{
  uint16_t Magic;
  uint8_t  MajorLinkerVersion;
  uint8_t  MinorLinkerVersion;
  uint32_t SizeOfCode;
  uint32_t SizeOfInitializedData;
  uint32_t SizeOfUninitializedData;
  uint32_t AddressOfEntryPoint;
  uint32_t BaseOfCode;
  uint32_t BaseOfData;
  uint32_t ImageBase;
  uint32_t SectionAlignment;
  uint32_t FileAlignment;
  uint16_t MajorOperatingSystemVersion;
  uint16_t MinorOperatingSystemVersion;
  uint16_t MajorImageVersion;
  uint16_t MinorImageVersion;
  uint16_t MajorSubsystemVersion;
  uint16_t MinorSubsystemVersion;
  uint32_t Win32VersionValue;
  uint32_t SizeOfImage;
  uint32_t SizeOfHeaders;
  uint32_t CheckSum;
  uint16_t Subsystem;
  uint16_t Dllcharacteristics;
  uint32_t SizeOfStackReserve;
  uint32_t SizeOfStackCommit;
  uint32_t SizeOfHeapReserve;
  uint32_t SizeOfHeapCommit;
  uint32_t LoaderFlags;
  uint32_t NumberOfRvaAndSizes;
  DataDirectory DataDirectories[NumberOfDataDirectories];
};

struct OptionalHeader64
{
  uint16_t Magic;
  uint8_t  MajorLinkerVersion;
  uint8_t  MinorLinkerVersion;
  uint32_t SizeOfCode;
  uint32_t SizeOfInitializedData;
  uint32_t SizeOfUninitializedData;
  uint32_t AddressOfEntryPoint;
  uint32_t BaseOfCode;
  uint64_t ImageBase;
  uint32_t SectionAlignment;
  uint32_t FileAlignment;
  uint16_t MajorOperatingSystemVersion;
  uint16_t MinorOperatingSystemVersion;
  uint16_t MajorImageVersion;
  uint16_t MinorImageVersion;
  uint16_t MajorSubsystemVersion;
  uint16_t MinorSubsystemVersion;
  uint32_t Win32VersionValue;
  uint32_t SizeOfImage;
  uint32_t SizeOfHeaders;
  uint32_t CheckSum;
  uint16_t Subsystem;
  uint16_t Dllcharacteristics;
  uint64_t SizeOfStackReserve;
  uint64_t SizeOfStackCommit;
  uint64_t SizeOfHeapReserve;
  uint64_t SizeOfHeapCommit;
  uint32_t LoaderFlags;
  uint32_t NumberOfRvaAndSizes;
  DataDirectory DataDirectories[NumberOfDataDirectories];
};

// Directory entry types

enum {
  IMAGE_DIRECTORY_ENTRY_EXPORT = 0,
  IMAGE_DIRECTORY_ENTRY_IMPORT = 1
};

struct PEHeader32
{
  uint32_t Signature;
  COFFHeader CoffHeader;
  OptionalHeader64 OptionalHeader;
};

struct PEHeader64
{
  uint32_t Signature;
  COFFHeader CoffHeader;
  OptionalHeader32 OptionalHeader;
};

struct SectionHeader
{
  char     Name[NameSize];
  uint32_t VirtualSize;
  uint32_t VirtualAddress;
  uint32_t SizeOfRawData;
  uint32_t PointerToRawData;
  uint32_t PointerToRelocations;
  uint32_t PointerToLinenumbers;
  uint16_t NumberOfRelocations;
  uint16_t NumberOfLinenumbers;
  uint32_t Characteristics;
};

struct ExportDirectory
{
  uint32_t ExportFlags;
  uint32_t TimeDateStamp;
  uint16_t MajorVersion;
  uint16_t MinorVersion;
  uint32_t Name; // RVA
  uint32_t Base;
  uint32_t NumberOfFuncions;
  uint32_t NumberOfNames;
  uint32_t AddressOfFunctions; // RVA from base of image
  uint32_t AddressOfNames; // RVA from base of image
  uint32_t AddressOfNameOrdinals; // RVA from base of image
};

struct IMAGE_IMPORT_DESCRIPTOR
{
  uint32_t AddressOfLookupTable; // RVA
  uint32_t TimeDateStamp;
  uint32_t ForwarderChain;
  uint32_t Name; // RVA from base of image
  uint32_t AddressOfImportAddressTable;
};

}

#endif /* CPPMICROSERVICES_PE_H */
