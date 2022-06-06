/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Xiangyi Meng <xymeng16@gmail.com>
 *
 * Copyright (c) 2022. All rights reserved.
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
#include <uk/sgx_cpu.h>
#include <uk/sgx_asm.h>
#include <uk/sgx_internal.h>
#include <uk/print.h>
#include <vfscore/uio.h>

#include <stdbool.h>

#define DEV_SGX_NAME "sgx"

#define SGX_MAX_EPC_BANKS 8 /* support up to 8 EPC banks according to Intel SGX  \
			     OOT-drvier 2.11 */
struct sgx_epc_bank sgx_epc_banks[SGX_MAX_EPC_BANKS];
int sgx_nr_epc_banks;
__u64 sgx_encl_size_max_64;
bool sgx_has_sgx2;

static int sgx_reset_pubkey_hash() {
	/* 
	 * TODO: linux-sgx-driver uses those hard-coded values, but in-kernel driver
	 * uses generated ones. This implementation follows the older one and should be optimized.
	 */
	if (wrmsrl_safe(MSR_IA32_SGXLEPUBKEYHASH0, 0xa6053e051270b7acULL) ||
		wrmsrl_safe(MSR_IA32_SGXLEPUBKEYHASH1, 0x6cfbe8ba8b3b413dULL) ||
		wrmsrl_safe(MSR_IA32_SGXLEPUBKEYHASH2, 0xc4916d99f2b3735dULL) ||
		wrmsrl_safe(MSR_IA32_SGXLEPUBKEYHASH3, 0xd4f8c05909f9bb3bULL)) {
			return -EIO;
		}
	return 0;
}

int sgx_init()
{
	unsigned int info[4] = {0, 0, 0, 0};
	unsigned int *eax, *ebx, *ecx, *edx;
	unsigned long pa, size;
	int i, ret;

	eax = &info[0];
	ebx = &info[1];
	ecx = &info[2];
	edx = &info[3];

	__cpuid_count(info, SGX_CPUID, SGX_CPUID_CAPABILITIES);

	sgx_encl_size_max_64 = 1ULL << ((*edx >> 8) & 0xFF);

	/* initialize EPC banks */

	for (i = 0; i < SGX_MAX_EPC_BANKS; i++) {
		__cpuid_count(info, SGX_CPUID, i + SGX_CPUID_EPC_BANKS);
		if (!(*eax & 0xf))
			break;

		pa = ((__u64)(*ebx & 0xfffff) << 32)
		     + (__u64)(*eax & 0xfffff000);
		size = ((__u64)(*edx & 0xfffff) << 32)
		       + (__u64)(*ecx & 0xfffff000);

		uk_pr_info("EPC section: 0x%lx-0x%lx (%ldMB)\n", pa, pa + size, size / 1024 / 1024);

		sgx_epc_banks[i].pa = pa;
		sgx_epc_banks[i].size = size;
	}

	sgx_nr_epc_banks = i;

	/* initialize sgx_free_list_lock */
	ukarch_spin_init(&sgx_free_list_lock);

	/* 
	 * set virtual address the same as the physical address 
	 * TODO: linux-sgx-driver uses ioremap() to map the physical address to 
	 * virtual address, but in Unikraft, Virtual Memory API is a optional feature in 
	 * Platform Interface Options, hence, we need to add two conditions here, like:
	 * #ifdef CONFIG_PAGING
	 * code to handle PA-VA mapping...
	 * #else
	 * code for PA only...
	 * #endif
	 */
	for (i = 0; i < sgx_nr_epc_banks; i++) {
		sgx_epc_banks[i].va = pa;
		ret = sgx_add_epc_bank(sgx_epc_banks[i].pa, sgx_epc_banks[i].size, i);

		if (ret) {
			sgx_nr_epc_banks = i + 1;
			goto out_iounmap;
		}
	}

	ret = sgx_page_cache_init();
	if (ret) {
		uk_pr_err("sgx_page_cache_init() failed with return value %d\n", ret);
		goto out_iounmap;
	}
	
	/* 
	 * TODO: for paging, linux-sgx-driver alloc a workqueue here, need to implement something similar once
	 * Unikraft supports SMP.
	 */

	/* 
	 * MSR_IA32_SGXLEPUBKEYHASHn is only writable if FLC is supported,
	 * so we need to check before reset the hash values.
	 */
	unsigned long fc = rdmsrl(X86_MSR_IA32_FEAT_CTL);
	if (fc & X86_FEAT_CTL_SGX_LC_ENABLED) {
		ret = sgx_reset_pubkey_hash();
			if (ret) {
				uk_pr_err("can not reset SGX LE public key hash MSRs dut to unknown reason\n");
		}
	} else {
		uk_pr_info("Flexible Launch Control is not enabled\n");
	}

	return 0;

out_iounmap:
	uk_pr_err("dummy iounmap\n");

	return ret;
}

int sgx_probe()
{
	unsigned int info[4] = {0, 0, 0, 0};
	unsigned int *eax, *ebx, *ecx, *edx;
	unsigned long fc;

	eax = &info[0];
	ebx = &info[1];
	ecx = &info[2];
	edx = &info[3];

	/* Is this an Intel CPU? */
	__cpuid_count(info, 0x00, 0);
	if (*ebx != Genu || *ecx != ntel || *edx != ineI)
		return 0;

	/* Does the CPU support Intel SGX? */
	__cpuid_count(info, 0x07, 0);
	if (!(*ebx & (0x1 << 2))) { /* Intel SGX is not supported by this CPU */
		uk_pr_err("Intel SGX is not supported by this CPU\n");
		goto error_exit;
	}

	fc = rdmsrl(X86_MSR_IA32_FEAT_CTL);

	if (!(fc & X86_FEAT_CTL_LOCKED)) {
		uk_pr_err("The feature control MSR is not locked\n");
		goto error_exit;
	}

	/* Is Intel SGX enabled? */
	if (!(fc & X86_FEAT_CTL_SGX_ENABLED)) {
		uk_pr_err("Intel SGX is not enabled in the BIOS\n");
		goto error_exit;
	}

	__cpuid(info, 0x00);
	if (*eax < SGX_CPUID) {
		uk_pr_err("CPUID is missing the SGX leaf\n");
		goto error_exit;
	}

	__cpuid_count(info, SGX_CPUID, SGX_CPUID_CAPABILITIES);
	if (!(*eax & 0x1)) {
		uk_pr_err("This CPU does not support the SGX1 instructions\n");
		goto error_exit;
	}
	sgx_has_sgx2 = ((*eax & 0x2) != 0);

	uk_pr_info("SGX status: SGX1 - true, SGX2 - %s\n",
		   sgx_has_sgx2 ? "true" : "false");


	uk_pr_info("Current machine is running under %s mode\n", read_cr0() & 0x1 ? "protected" : "real");
	
	uk_pr_info("Paging is %s\n", read_cr0() & 0x80000000 ? "enabled" : "disabled");

	return sgx_init();

error_exit:
	return -ENODEV;
}

static struct devops sgx_devops = {
	.ioctl = sgx_ioctl,
	.open = sgx_open,
	.close = sgx_close,
};

static struct driver drv_sgx = {
	.devops = &sgx_devops,
	.devsz = 0,
	.name = DEV_SGX_NAME
};

static int devfs_register(void) 
{
	struct device *dev;

	uk_pr_info("Register %s to devfs\n", DEV_SGX_NAME);

	/* register /dev/sgx */
	dev = device_create(&drv_sgx, DEV_SGX_NAME, D_CHR);
	if (dev == NULL) {
		uk_pr_err("Failed to register %s to devfs\n", DEV_SGX_NAME);
		return -1;
	}

	return 0;
}

devfs_initcall(devfs_register);