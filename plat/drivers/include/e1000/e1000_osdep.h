/******************************************************************************

  Copyright (c) 2001-2014, Intel Corporation 
  All rights reserved.
  
  Redistribution and use in source and binary forms, with or without 
  modification, are permitted provided that the following conditions are met:
  
   1. Redistributions of source code must retain the above copyright notice, 
      this list of conditions and the following disclaimer.
  
   2. Redistributions in binary form must reproduce the above copyright 
      notice, this list of conditions and the following disclaimer in the 
      documentation and/or other materials provided with the distribution.
  
   3. Neither the name of the Intel Corporation nor the names of its 
      contributors may be used to endorse or promote products derived from 
      this software without specific prior written permission.
  
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.

******************************************************************************/
/*$FreeBSD$*/

#ifndef _E1000_OSDEP_H_
#define _E1000_OSDEP_H_

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include <uk/netdev_core.h>
#include <uk/arch/types.h>
#include <uk/plat/common/cpu.h>
#include <uk/netdev.h>

#include "e1000_hw.h"

#define DELAY(x) usleep(x)
#define usec_delay(x) DELAY(x)
#define usec_delay_irq(x) DELAY(x)
#define msec_delay(x) DELAY((x))
// #define msec_delay(x) DELAY(1000*(x))
#define msec_delay_irq(x) DELAY((x))
// #define msec_delay_irq(x) DELAY(1000*(x))

#define UNREFERENCED_PARAMETER(_p)
#define UNREFERENCED_1PARAMETER(_p)
#define UNREFERENCED_2PARAMETER(_p, _q)
#define UNREFERENCED_3PARAMETER(_p, _q, _r)
#define UNREFERENCED_4PARAMETER(_p, _q, _r, _s)

#define FALSE			0
#define TRUE			1

#define	CMD_MEM_WRT_INVALIDATE	0x0010  /* BIT_4 */

/* Mutex used in the shared code */
#define E1000_MUTEX                     uintptr_t
#define E1000_MUTEX_INIT(mutex)         (*(mutex) = 0)
#define E1000_MUTEX_LOCK(mutex)         (*(mutex) = 1)
#define E1000_MUTEX_UNLOCK(mutex)       (*(mutex) = 0)

typedef uint64_t	__u64;
typedef uint32_t	__u32;
typedef uint16_t	__u16;
typedef uint8_t		__u8;
typedef int64_t		__s64;
typedef int32_t		__s32;
typedef int16_t		__s16;
typedef int8_t		__s8;

#define __le16		__u16
#define __le32		__u32
#define __le64		__u64

#define E1000_WRITE_FLUSH(a) E1000_READ_REG(a, E1000_STATUS)

#define E1000_PCI_REG(reg) (*((volatile uint32_t *)(reg)))

#define E1000_PCI_REG_WRITE(reg, value)			\
	do { \
		E1000_PCI_REG((reg)) = value; \
	} while(0)

#define E1000_PCI_REG_WRITE_RELAXED(reg, value)		\
	do { \
		E1000_PCI_REG((reg)) = value; \
	} while(0);

#define E1000_PCI_REG_ADDR(hw, reg) \
	((volatile uint32_t *)((char *)(hw)->hw_addr + (reg)))

#define E1000_PCI_REG_ARRAY_ADDR(hw, reg, index) \
	E1000_PCI_REG_ADDR((hw), (reg) + ((index) << 2))

static inline uint32_t e1000_read_addr(volatile void *addr, int reg)
{
	uint64_t ret;

	ret = *(uint32_t *)(addr + reg);

	return ret;
}

static inline void e1000_write_addr(volatile void *addr, __u32 reg, __u32 value)
{
	*(uint32_t *)(addr + reg) = value;
}

/* Necessary defines */
#define E1000_MRQC_ENABLE_MASK                  0x00000007
#define E1000_MRQC_RSS_FIELD_IPV6_EX		0x00080000
#define E1000_ALL_FULL_DUPLEX   ( \
        ADVERTISE_10_FULL | ADVERTISE_100_FULL | ADVERTISE_1000_FULL)

#define M88E1543_E_PHY_ID    0x01410EA0
#define ULP_SUPPORT

#define E1000_RCTL_DTYP_MASK	0x00000C00 /* Descriptor type mask */
#define E1000_MRQC_RSS_FIELD_IPV6_EX            0x00080000

/* Register READ/WRITE macros */


#define PCI_CONF_READ(type, ret, a, s)					\
	do {								\
		uint32_t _conf_data;					\
		outl(PCI_CONFIG_ADDR, (a) | PCI_CONF_##s);		\
		_conf_data = ((inl(PCI_CONFIG_DATA) >> PCI_CONF_##s##_SHFT) \
			      & PCI_CONF_##s##_MASK);			\
		*(ret) = (type) _conf_data;				\
	} while (0)


#define E1000_READ_REG(hw, reg) \
	e1000_read_addr((volatile void *) ((uint64_t) hw->pdev->bar0 & 0xFFFFFFF0), (reg))


#define E1000_WRITE_REG(hw, reg, value) \
	e1000_write_addr((volatile void *) ((uint64_t) hw->pdev->bar0 & 0xFFFFFFF0), reg, (value))


#define E1000_WRITE_REG_ARRAY(hw, reg, index, value) \
	E1000_PCI_REG_WRITE(E1000_PCI_REG_ARRAY_ADDR((hw), (reg), (index)), (value)) // TODO

#define E1000_READ_REG_ARRAY(hw, reg, index) \
	E1000_PCI_REG(E1000_PCI_REG_ARRAY_ADDR((hw), (reg), (index))) // TODO

/*
#define E1000_READ_REG_ARRAY_DWORD E1000_READ_REG_ARRAY
#define E1000_WRITE_REG_ARRAY_DWORD E1000_WRITE_REG_ARRAY
*/

/*
 * To be able to do IO write, we need to map IO BAR
 * (bar 2/4 depending on device).
 * Right now mapping multiple BARs is not supported by DPDK.
 * Fortunatelly we need it only for legacy hw support.
 */

/*
#define E1000_WRITE_REG_IO(hw, reg, value) \
	E1000_WRITE_REG(hw, reg, value)
*/

#define STATIC static

#ifndef ETH_ADDR_LEN
#define ETH_ADDR_LEN                  6
#endif

#endif /* _E1000_OSDEP_H_ */
