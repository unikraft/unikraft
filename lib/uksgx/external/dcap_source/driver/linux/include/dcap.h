// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
// Copyright(c) 2016-21 Intel Corporation.

#ifndef __DCAP_H__
#define __DCAP_H__

#ifndef X86_FEATURE_SGX
	#define X86_FEATURE_SGX				(9 * 32 + 2)
#endif

#ifndef X86_FEATURE_SGX1
	#define X86_FEATURE_SGX1         ( 8*32+ 0) /* SGX1 leaf functions */
#endif

#ifndef X86_FEATURE_SGX2
	#define X86_FEATURE_SGX2         ( 8*32+ 1) /* SGX2 leaf functions */
#endif

#ifndef X86_FEATURE_SGX_LC
	#define X86_FEATURE_SGX_LC			(16*32+30) /* supports SGX launch configuration */
#endif

#ifndef FEAT_CTL_SGX_ENABLED
	#define FEAT_CTL_SGX_ENABLED                             (1<<18)
#endif

#ifndef FEAT_CTL_SGX_LC_ENABLED
	#define FEAT_CTL_SGX_LC_ENABLED                         (1<<17)
#endif

#ifndef MSR_IA32_SGXLEPUBKEYHASH0
    #define MSR_IA32_SGXLEPUBKEYHASH0	0x0000008C
    #define MSR_IA32_SGXLEPUBKEYHASH1	0x0000008D
    #define MSR_IA32_SGXLEPUBKEYHASH2	0x0000008E
    #define MSR_IA32_SGXLEPUBKEYHASH3	0x0000008F
#endif


#endif
