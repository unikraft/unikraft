#include <uk/arch/types.h>

struct RSDPDescriptor {
	char Signature[8];
	__u8 Checksum;
	char OEMID[6];
	__u8 Revision;
	__u32 RsdtAddress;
} __attribute__((packed));

struct RSDPDescriptor20 {
	RSDPDescriptor firstPart;

	__u32 Length;
	__u64 XsdtAddress;
	__u8 ExtendedChecksum;
	__u8 reserved[3];
} __attribute__((packed));

struct ACPISDTHeader {
	char Signature[4];
	__u32 Length;
	__u8 Revision;
	__u8 Checksum;
	char OEMID[6];
	char OEMTableID[8];
	__u32 OEMRevision;
	__u32 CreatorID;
	__u32 CreatorRevision;
};

char RSDTChecksum(ACPISDTHeader *header);

char RSDP20Checksum(RSDPDescriptor20 *str);

char RSDPChecksum(RSDPDescriptor *str);