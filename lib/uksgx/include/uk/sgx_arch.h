#ifndef _UK_SGX_ARCH_H_
#define _UK_SGX_ARCH_H_

#include <uk/arch/types.h>

#define SGX_SSA_GPRS_SIZE		182
#define SGX_SSA_MISC_EXINFO_SIZE	16

enum sgx_misc {
	SGX_MISC_EXINFO		= 0x01,
};

#define SGX_MISC_RESERVED_MASK 0xFFFFFFFFFFFFFFFEL

enum sgx_attribute {
	SGX_ATTR_DEBUG		= 0x02,
	SGX_ATTR_MODE64BIT	= 0x04,
	SGX_ATTR_PROVISIONKEY	= 0x10,
	SGX_ATTR_EINITTOKENKEY	= 0x20,
};

#define SGX_ATTR_RESERVED_MASK 0xFFFFFFFFFFFFFF49L

#define SGX_SECS_RESERVED1_SIZE 24
#define SGX_SECS_RESERVED2_SIZE 32
#define SGX_SECS_RESERVED3_SIZE 32
#define SGX_SECS_RESERVED4_SIZE 3834

struct sgx_secs {
	__u64 size;
	__u64 base;
	__u32 ssaframesize;
	__u32 miscselect;
	__u8 reserved1[SGX_SECS_RESERVED1_SIZE];
	__u64 attributes;
	__u64 xfrm;
	__u32 mrenclave[8];
	__u8 reserved2[SGX_SECS_RESERVED2_SIZE];
	__u32 mrsigner[8];
	__u32 configid[16];
	__u8	reserved3[SGX_SECS_RESERVED3_SIZE];
	__u16 isvvprodid;
	__u16 isvsvn;
	__u16 configsvn;
	__u8 reserved4[SGX_SECS_RESERVED4_SIZE];
};

enum sgx_tcs_flags {
	SGX_TCS_DBGOPTIN	= 0x01, /* cleared on EADD */
};

#define SGX_TCS_RESERVED_MASK 0xFFFFFFFFFFFFFFFEL

struct sgx_tcs {
	__u64 state;
	__u64 flags;
	__u64 ossa;
	__u32 cssa;
	__u32 nssa;
	__u64 oentry;
	__u64 aep;
	__u64 ofsbase;
	__u64 ogsbase;
	__u32 fslimit;
	__u32 gslimit;
	__u64 reserved[503];
};

struct sgx_pageinfo {
	__u64 linaddr;
	__u64 srcpge;
	union {
		__u64 secinfo;
		__u64 pcmd;
	};
	__u64 secs;
} __attribute__((aligned(32)));


#define SGX_SECINFO_PERMISSION_MASK	0x0000000000000007L
#define SGX_SECINFO_PAGE_TYPE_MASK	0x000000000000FF00L
#define SGX_SECINFO_RESERVED_MASK	0xFFFFFFFFFFFF00F8L

enum sgx_page_type {
	SGX_PAGE_TYPE_SECS	= 0x00,
	SGX_PAGE_TYPE_TCS	= 0x01,
	SGX_PAGE_TYPE_REG	= 0x02,
	SGX_PAGE_TYPE_VA	= 0x03,
};

enum sgx_secinfo_flags {
	SGX_SECINFO_R		= 0x01,
	SGX_SECINFO_W		= 0x02,
	SGX_SECINFO_X		= 0x04,
	SGX_SECINFO_SECS	= (SGX_PAGE_TYPE_SECS << 8),
	SGX_SECINFO_TCS		= (SGX_PAGE_TYPE_TCS << 8),
	SGX_SECINFO_REG		= (SGX_PAGE_TYPE_REG << 8),
};

struct sgx_secinfo {
	__u64 flags;
	__u64 reserved[7];
} __attribute__((aligned(64)));

struct sgx_pcmd {
	struct sgx_secinfo secinfo;
	__u64 enclave_id;
	__u8 reserved[40];
	__u8 mac[16];
};

#define SGX_MODULUS_SIZE 384

struct sgx_sigstruct_header {
	__u64 header1[2];
	__u32 vendor;
	__u32 date;
	__u64 header2[2];
	__u32 swdefined;
	__u8 reserved1[84];
};

struct sgx_sigstruct_body {
	__u32 miscselect;
	__u32 miscmask;
	__u8 reserved2[20];
	__u64 attributes;
	__u64 xfrm;
	__u8 attributemask[16];
	__u8 mrenclave[32];
	__u8 reserved3[32];
	__u16 isvprodid;
	__u16 isvsvn;
} __attribute__((__packed__));

struct sgx_sigstruct {
	struct sgx_sigstruct_header header;
	__u8 modulus[SGX_MODULUS_SIZE];
	__u32 exponent;
	__u8 signature[SGX_MODULUS_SIZE];
	struct sgx_sigstruct_body body;
	__u8 reserved4[12];
	__u8 q1[SGX_MODULUS_SIZE];
	__u8 q2[SGX_MODULUS_SIZE];
};

struct sgx_sigstruct_payload {
	struct sgx_sigstruct_header header;
	struct sgx_sigstruct_body body;
};

struct sgx_einittoken_payload {
	__u32 valid;
	__u32 reserved1[11];
	__u64 attributes;
	__u64 xfrm;
	__u8 mrenclave[32];
	__u8 reserved2[32];
	__u8 mrsigner[32];
	__u8 reserved3[32];
};

struct sgx_einittoken {
	struct sgx_einittoken_payload payload;
	__u8 cpusvnle[16];
	__u16 isvprodidle;
	__u16 isvsvnle;
	__u8 reserved2[24];
	__u32 maskedmiscselectle;
	__u64 maskedattributesle;
	__u64 maskedxfrmle;
	__u8 keyid[32];
	__u8 mac[16];
};

struct sgx_report {
	__u8 cpusvn[16];
	__u32 miscselect;
	__u8 reserved1[28];
	__u64 attributes;
	__u64 xfrm;
	__u8 mrenclave[32];
	__u8 reserved2[32];
	__u8 mrsigner[32];
	__u8 reserved3[96];
	__u16 isvprodid;
	__u16 isvsvn;
	__u8 reserved4[60];
	__u8 reportdata[64];
	__u8 keyid[32];
	__u8 mac[16];
};

struct sgx_targetinfo {
	__u8 mrenclave[32];
	__u64 attributes;
	__u64 xfrm;
	__u8 reserved1[4];
	__u32 miscselect;
	__u8 reserved2[456];
};

struct sgx_keyrequest {
	__u16 keyname;
	__u16 keypolicy;
	__u16 isvsvn;
	__u16 reserved1;
	__u8 cpusvn[16];
	__u64 attributemask;
	__u64 xfrmmask;
	__u8 keyid[32];
	__u32 miscmask;
	__u8 reserved2[436];
};

#endif