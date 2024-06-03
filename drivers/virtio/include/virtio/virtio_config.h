/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Sharan Santhanam <sharan.santhanam@neclab.eu>
 *
 * Copyright (c) 2018, NEC Europe Ltd., NEC Corporation. All rights reserved.
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
 */

#ifndef __VIRTIO_CONFIG_H__
#define __VIRTIO_CONFIG_H__

#include <uk/arch/types.h>
#include <uk/config.h>
#include <uk/plat/common/cpu.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus __ */

#define MAX_TRY_COUNT				10

#define VIRTIO_CONFIG_STATUS_RESET		0x0  /* device reset */
#define VIRTIO_CONFIG_STATUS_ACK		0x1  /* device is virtio */
#define VIRTIO_CONFIG_STATUS_DRIVER		0x2  /* driver found */
#define VIRTIO_CONFIG_STATUS_DRIVER_OK		0x4  /* init complete */
#define VIRTIO_CONFIG_STATUS_FEATURES_OK	0x8  /* feat negot complete */
#define VIRTIO_CONFIG_STATUS_NEEDS_RESET	0x40 /* device needs reset */
#define VIRTIO_CONFIG_STATUS_FAIL		0x80 /* device failure */

#define VIRTIO_TRANSPORT_F_START		28
#define VIRTIO_TRANSPORT_F_END			32

/* v1.0 compliant. */
#define VIRTIO_F_VERSION_1			32

#if CONFIG_ARCH_X86_64
static inline void virtio_cwrite_bytes(const void *addr, const __u8 offset,
				       const void *buf, int len, int type_len)
{
	int i = 0;
	__u16 io_addr;
	int count;

	count  = len / type_len;
	for (i = 0; i < count; i++) {
		io_addr = ((unsigned long)addr) + offset + (i * type_len);
		switch (type_len) {
		case 1:
			outb(io_addr, ((__u8 *)buf)[i * type_len]);
			break;
		case 2:
			outw(io_addr, ((__u16 *)buf)[i * type_len]);
			break;
		case 4:
			outl(io_addr, ((__u32 *)buf)[i * type_len]);
			break;
		default:
			UK_CRASH("Unsupported virtio write operation\n");
		}
	}
}

static inline void virtio_cread_bytes(const void *addr, const __u8 offset,
				      void *buf, int len, int type_len)
{
	int i = 0;
	__u16 io_addr;
	int count;

	count = len / type_len;
	for (i = 0; i < count; i++) {
		io_addr = ((unsigned long)addr) + offset + (i * type_len);
		switch (type_len) {
		case 1:
			((__u8 *)buf)[i * type_len] = inb(io_addr);
			break;
		case 2:
			((__u16 *)buf)[i * type_len] = inw(io_addr);
			break;
		case 4:
			((__u32 *)buf)[i * type_len] = inl(io_addr);
			break;
		case 8:
			((__u64  *)buf)[i * type_len] = inq(io_addr);
			break;
		default:
			UK_CRASH("Unsupported virtio read operation\n");
		}
	}
}

static inline
void virtio_mmio_cwrite_bytes(const void *addr, const __u8 offset,
			      const void *buf, int len, int type_len)
{
	int i = 0;
	__u64 io_addr;
	int count;

	count  = len / type_len;
	for (i = 0; i < count; i++) {
		io_addr = ((unsigned long)addr) + offset + (i * type_len);
		/* HELP: Somehow when you remove this print, uksev does not
		 * emulate the write / interrupt is not triggered. */
		uk_pr_info("\n");
		switch (type_len) {
		case 1:
			writeb((__u8 *)io_addr, ((__u8 *)buf)[i * type_len]);
			break;
		case 2:
			writew((__u16 *)io_addr, ((__u16 *)buf)[i * type_len]);
			break;
		case 4:
			/* uk_pr_info("Data: 0x%x\n", ((__u32  *)buf)[i * type_len]); */
			writel((__u32 *)io_addr, ((__u32 *)buf)[i * type_len]);
			break;
		default:
			UK_CRASH("Unsupported virtio write operation\n");
		}
	}
}

static inline
void virtio_mmio_cread_bytes(const void *addr, const __u8 offset,
			     void *buf, int len, int type_len)
{
	int i = 0;
	__u64 io_addr;
	int count;

	count = len / type_len;
	for (i = 0; i < count; i++) {
		io_addr = ((unsigned long)addr) + offset + (i * type_len);
		switch (type_len) {
		case 1:
			((__u8 *)buf)[i * type_len] = readb((__u8 *)io_addr);
			break;
		case 2:
			((__u16 *)buf)[i * type_len] = readw((__u16 *)io_addr);
			break;
		case 4:
			((__u32 *)buf)[i * type_len] = readl((__u32 *)io_addr);
			break;
		case 8:
			((__u64  *)buf)[i * type_len] = readq((__u64  *)io_addr);
			break;
		default:
			UK_CRASH("Unsupported virtio read operation\n");
		}
	}
}
#else  /* !CONFIG_ARCH_X86_64 */

/* IO barriers */
#define __iormb()		rmb()
#define __iowmb()		wmb()

#define virtio_mmio_cwrite_bytes	virtio_cwrite_bytes
#define virtio_mmio_cread_bytes		virtio_cread_bytes

static inline void virtio_cwrite_bytes(const void *addr, const __u8 offset,
				       const void *buf, int len, int type_len)
{
	int i = 0;
	void *io_addr;
	int count;

	count  = len / type_len;
	for (i = 0; i < count; i++) {
		io_addr = (void *)addr + offset + (i * type_len);
		__iowmb();
		switch (type_len) {
		case 1:
			ioreg_write8(io_addr, ((__u8 *)buf)[i * type_len]);
			break;
		case 2:
			ioreg_write16(io_addr, ((__u16 *)buf)[i * type_len]);
			break;
		case 4:
			ioreg_write32(io_addr, ((__u32 *)buf)[i * type_len]);
			break;
		default:
			UK_CRASH("Unsupported virtio write operation\n");
		}
	}
}

static inline void virtio_cread_bytes(const void *addr, const __u8 offset,
				      void *buf, int len, int type_len)
{
	int i = 0;
	void *io_addr;
	int count;

	count = len / type_len;
	for (i = 0; i < count; i++) {
		io_addr = (void *)addr + offset + (i * type_len);
		switch (type_len) {
		case 1:
			((__u8 *)buf)[i * type_len] = ioreg_read8(io_addr);
			break;
		case 2:
			((__u16 *)buf)[i * type_len] = ioreg_read16(io_addr);
			break;
		case 4:
			((__u32 *)buf)[i * type_len] = ioreg_read32(io_addr);
			break;
		default:
			UK_CRASH("Unsupported virtio read operation\n");
		}
		__iormb();
	}
}

#endif  /* !CONFIG_ARCH_X86_64 */

/**
 * Read the virtio device configuration of specified length.
 *
 * @param addr
 *   The base address of the device.
 * @param offset
 *   The offset with the device address space.
 * @param buf
 *    The destination buffer to which the value has to be copied.
 * @param len
 *    The length of the destination buffer.
 */
static inline int virtio_cread_bytes_many(const void *addr, const __u8 offset,
					  __u8 *buf, __u32 len)
{
	__u8 old_buf[len];
	int check;
	int cnt = 0;
	__u32 i = 0;

	do {
		check = len;
		virtio_cread_bytes(addr, offset, &old_buf[0], len, 1);
		virtio_cread_bytes(addr, offset, buf, len, 1);

		for (i = 0; i < len; i++) {
			if (unlikely(buf[i] != old_buf[i])) {
				check = -1; /* Need to retry configuration */
				break;
			}
		}
		cnt++;
	} while (check == -1 && cnt < MAX_TRY_COUNT);

	return check;
}

/**
 * Read the single byte configuration.
 *
 * @param addr
 *   The base address of the device.
 * @param offset
 *   The offset with the device address space.
 * @return __u8
 *   Returns the value configuration register.
 */
static inline __u8 virtio_cread8(const void *addr, const __u8 offset)
{
	__u8 buf = 0;

	virtio_cread_bytes(addr, offset, &buf, sizeof(buf), sizeof(buf));
	return buf;
}

/**
 * Read the single word configuration.
 *
 * @param addr
 *   The base address of the device.
 * @param offset
 *   The offset with the device address space.
 * @return __u16
 *   Returns the value configuration register.
 */
static inline __u16 virtio_cread16(const void *addr, const __u8 offset)
{
	__u16 buf = 0;

	virtio_cread_bytes(addr, offset, &buf, sizeof(buf), sizeof(buf));
	return buf;
}

/**
 * Read the single long word configuration.
 *
 * @param addr
 *   The base address of the device.
 * @param offset
 *   The offset with the device address space.
 * @return __u32
 *   Returns the value configuration register.
 */
static inline __u32 virtio_cread32(const void *addr, const __u8 offset)
{
	__u32 buf = 0;

	virtio_cread_bytes(addr, offset, &buf, sizeof(buf), sizeof(buf));
	return buf;
}

/**
 * Write the configuration.
 *
 * @param addr
 *   The base address of the device.
 * @param offset
 *   The offset with the device address space.
 * @param data
 *   The value to write to the configuration.
 */
static inline void virtio_cwrite8(const void *addr, const __u8 offset,
				  const __u8 data)
{
	virtio_cwrite_bytes(addr, offset, &data, sizeof(data), sizeof(data));
}

/**
 * Write the configuration.
 *
 * @param addr
 *   The base address of the device.
 * @param offset
 *   The offset with the device address space.
 * @param data
 *   The value to write to the configuration.
 */
static inline void virtio_cwrite16(const void *addr, const __u8 offset,
				   const __u16 data)
{
	virtio_cwrite_bytes(addr, offset, &data, sizeof(data), sizeof(data));
}

/**
 * Write the configuration.
 *
 * @param addr
 *   The base address of the device.
 * @param offset
 *   The offset with the device address space.
 * @param data
 *   The value to write to the configuration.
 */
static inline void virtio_cwrite32(const void *addr, const __u8 offset,
				   const __u32 data)
{
	virtio_cwrite_bytes(addr, offset, &data, sizeof(data), sizeof(data));
}

/**
 * Read an 8-bit item from the device's config space
 *
 * @param addr
 *   The base address of the device.
 * @param offset
 *   The offset with the device address space.
 * @return __u8
 *   Returns the value configuration register.
 */
static inline __u8 virtio_mmio_cread8(const void *addr, const __u8 offset)
{
	__u8 buf = 0;

	virtio_mmio_cread_bytes(addr, offset, &buf, sizeof(buf), sizeof(buf));

	return buf;
}

/**
 * Read a 16-bit item from the device's config space
 *
 * @param addr
 *   The base address of the device.
 * @param offset
 *   The offset with the device address space.
 * @return __u16
 *   Returns the value configuration register.
 */
static inline __u16 virtio_mmio_cread16(const void *addr, const __u8 offset)
{
	__u16 buf = 0;

	virtio_mmio_cread_bytes(addr, offset, &buf, sizeof(buf), sizeof(buf));

	return buf;
}

/**
 * Read a 32-bit item from the device's config space
 *
 * @param addr
 *   The base address of the device.
 * @param offset
 *   The offset with the device address space.
 * @return __u32
 *   Returns the value configuration register.
 */
static inline __u32 virtio_mmio_cread32(const void *addr, const __u8 offset)
{
	__u32 buf = 0;

	virtio_mmio_cread_bytes(addr, offset, &buf, sizeof(buf), sizeof(buf));

	return buf;
}

/**
 * Write an 8-bit item from the device's config space
 *
 * @param addr
 *   The base address of the device.
 * @param offset
 *   The offset with the device address space.
 * @param data
 *   The value to write to the configuration.
 */
static inline void virtio_mmio_cwrite8(const void *addr, const __u8 offset,
				       const __u8 data)
{
	virtio_mmio_cwrite_bytes(addr, offset, &data, sizeof(data),
				 sizeof(data));
}

/**
 * Write a 16-bit item from the device's config space
 *
 * @param addr
 *   The base address of the device.
 * @param offset
 *   The offset with the device address space.
 * @param data
 *   The value to write to the configuration.
 */
static inline void virtio_mmio_cwrite16(const void *addr, const __u8 offset,
					const __u16 data)
{
	virtio_mmio_cwrite_bytes(addr, offset, &data, sizeof(data),
				 sizeof(data));
}

/**
 * Write a 32-bit item from the device's config space
 *
 * @param addr
 *   The base address of the device.
 * @param offset
 *   The offset with the device address space.
 * @param data
 *   The value to write to the configuration.
 */
static inline void virtio_mmio_cwrite32(const void *addr, const __u8 offset,
					const __u32 data)
{
	virtio_mmio_cwrite_bytes(addr, offset, &data, sizeof(data),
				 sizeof(data));
}

#ifdef __cplusplus
}
#endif /* __cplusplus __ */

#endif /* __VIRTIO_CONFIG_H__ */
