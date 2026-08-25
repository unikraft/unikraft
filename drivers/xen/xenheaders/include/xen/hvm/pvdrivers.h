/* SPDX-License-Identifier: MIT */
/*
 * pvdrivers.h: Register of PV drivers product numbers.
 * Copyright (c) 2012, Citrix Systems Inc.
 */

#ifndef _XEN_PUBLIC_PVDRIVERS_H_
#define _XEN_PUBLIC_PVDRIVERS_H_

/*
 * This is the master registry of product numbers for
 * PV drivers.
 * If you need a new product number allocating, please
 * post to xen-devel@lists.xenproject.org.  You should NOT use
 * a product number without allocating one.
 * If you maintain a separate versioning and distribution path
 * for PV drivers you should have a separate product number so
 * that your drivers can be separated from others.
 *
 * During development, you may use the product ID to
 * indicate a driver which is yet to be released.
 */

#define PVDRIVERS_PRODUCT_LIST(EACH)                               \
        EACH("xensource-windows",       0x0001) /* Citrix */       \
        EACH("gplpv-windows",           0x0002) /* James Harper */ \
        EACH("linux",                   0x0003)                    \
        EACH("xenserver-windows-v7.0+", 0x0004) /* Citrix */       \
        EACH("xenserver-windows-v7.2+", 0x0005) /* Citrix */       \
        EACH("experimental",            0xffff)

#endif /* _XEN_PUBLIC_PVDRIVERS_H_ */
