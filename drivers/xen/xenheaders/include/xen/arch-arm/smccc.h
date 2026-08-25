/* SPDX-License-Identifier: MIT */
/*
 * smccc.h
 *
 * SMC/HVC interface in accordance with SMC Calling Convention.
 *
 * Copyright 2017 (C) EPAM Systems
 */

#ifndef __XEN_PUBLIC_ARCH_ARM_SMCCC_H__
#define __XEN_PUBLIC_ARCH_ARM_SMCCC_H__

#include "public/xen.h"

/*
 * Hypervisor Service version.
 *
 * We can't use XEN version here, because of SMCCC requirements:
 * Major revision should change every time SMC/HVC function is removed.
 * Minor revision should change every time SMC/HVC function is added.
 * So, it is SMCCC protocol revision code, not XEN version.
 *
 * Those values are subjected to change, when interface will be extended.
 */
#define XEN_SMCCC_MAJOR_REVISION 0
#define XEN_SMCCC_MINOR_REVISION 1

/* Hypervisor Service UID. Randomly generated with uuidgen. */
#define XEN_SMCCC_UID XEN_DEFINE_UUID(0xa71812dcU, 0xc698, 0x4369, 0x9acf, \
                                      0x79, 0xd1, 0x8d, 0xde, 0xe6, 0x67)

/* Standard Service Service Call version. */
#define SSSC_SMCCC_MAJOR_REVISION 0
#define SSSC_SMCCC_MINOR_REVISION 1

/* Standard Service Call UID. Randomly generated with uuidgen. */
#define SSSC_SMCCC_UID XEN_DEFINE_UUID(0xf863386fU, 0x4b39, 0x4cbd, 0x9220,\
                                       0xce, 0x16, 0x41, 0xe5, 0x9f, 0x6f)

#endif /* __XEN_PUBLIC_ARCH_ARM_SMCCC_H__ */

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:b
 */
