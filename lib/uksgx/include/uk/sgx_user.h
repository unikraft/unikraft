#ifndef _SGX_USER_H
#define _SGX_USER_H

#include <uk/sgx.h>
#include <uk/arch/types.h>
#include <lib/nolibc/musl-imported/arch/generic/bits/ioctl.h>

#define SGX_MAGIC 0xA4

#define SGX_IOC_ENCLAVE_CREATE \
	_IOW(SGX_MAGIC, 0x00, struct sgx_enclave_create)
#define SGX_IOC_ENCLAVE_ADD_PAGE \
	_IOW(SGX_MAGIC, 0x01, struct sgx_enclave_add_page)
#define SGX_IOC_ENCLAVE_INIT \
	_IOW(SGX_MAGIC, 0x02, struct sgx_enclave_init)
#define SGX_IOC_ENCLAVE_EMODPR \
	_IOW(SGX_MAGIC, 0x09, struct sgx_modification_param)
#define SGX_IOC_ENCLAVE_MKTCS \
	_IOW(SGX_MAGIC, 0x0a, struct sgx_range)
#define SGX_IOC_ENCLAVE_TRIM \
	_IOW(SGX_MAGIC, 0x0b, struct sgx_range)
#define SGX_IOC_ENCLAVE_NOTIFY_ACCEPT \
	_IOW(SGX_MAGIC, 0x0c, struct sgx_range)
#define SGX_IOC_ENCLAVE_PAGE_REMOVE \
	_IOW(SGX_MAGIC, 0x0d, unsigned long)

/* SGX leaf instruction return values */
#define SGX_SUCCESS			0
#define SGX_INVALID_SIG_STRUCT		1
#define SGX_INVALID_ATTRIBUTE		2
#define SGX_BLKSTATE			3
#define SGX_INVALID_MEASUREMENT		4
#define SGX_NOTBLOCKABLE		5
#define SGX_PG_INVLD			6
#define SGX_LOCKFAIL			7
#define SGX_INVALID_SIGNATURE		8
#define SGX_MAC_COMPARE_FAIL		9
#define SGX_PAGE_NOT_BLOCKED		10
#define SGX_NOT_TRACKED			11
#define SGX_VA_SLOT_OCCUPIED		12
#define SGX_CHILD_PRESENT		13
#define SGX_ENCLAVE_ACT			14
#define SGX_ENTRYEPOCH_LOCKED		15
#define SGX_INVALID_EINITTOKEN		16
#define SGX_PREV_TRK_INCMPL		17
#define SGX_PG_IS_SECS			18
#define SGX_PAGE_NOT_MODIFIABLE		20
#define SGX_INVALID_CPUSVN		32
#define SGX_INVALID_ISVSVN		64
#define SGX_UNMASKED_EVENT		128
#define SGX_INVALID_KEYNAME		256

/* IOCTL return values */
#define SGX_POWER_LOST_ENCLAVE		0x40000000
#define SGX_LE_ROLLBACK			0x40000001

/**
 * struct sgx_enclave_create - parameter structure for the
 *                             %SGX_IOC_ENCLAVE_CREATE ioctl
 * @src:	address for the SECS page data
 */
struct sgx_enclave_create  {
	__u64	src;
} __attribute__((__packed__));

/**
 * struct sgx_enclave_add_page - parameter structure for the
 *                               %SGX_IOC_ENCLAVE_ADD_PAGE ioctl
 * @addr:	address in the ELRANGE
 * @src:	address for the page data
 * @secinfo:	address for the SECINFO data
 * @mrmask:	bitmask for the 256 byte chunks that are to be measured
 */
struct sgx_enclave_add_page {
	__u64	addr;
	__u64	src;
	__u64	secinfo;
	__u16	mrmask;
} __attribute__((__packed__));

/**
 * struct sgx_enclave_init - parameter structure for the
 *                           %SGX_IOC_ENCLAVE_INIT ioctl
 * @addr:	address in the ELRANGE
 * @sigstruct:	address for the page data
 * @einittoken:	EINITTOKEN
 */
struct sgx_enclave_init {
	__u64	addr;
	__u64	sigstruct;
	__u64	einittoken;
} __attribute__((__packed__));

/*
 *     SGX2.0 definitions
 */

struct sgx_range {
	unsigned long start_addr;
	unsigned int nr_pages;
};

struct sgx_modification_param {
	struct sgx_range range;
	unsigned long flags;
};


#endif