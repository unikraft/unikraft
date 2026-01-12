/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2018, Arm Ltd. All rights reserved.
 * Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */
#ifndef __UK_ASM_H__
#define __UK_ASM_H__

#if __ASSEMBLY__
#ifndef ENTRY_ALIGNED
#define ENTRY_ALIGNED(_name, _align)					\
	.global _name;							\
	.align _align;							\
	_name:
#endif

#ifndef ENTRY
#define ENTRY(_name)		ENTRY_ALIGNED(_name, 2)
#endif

#ifndef END
#define END(_name)							\
	.size _name, . - _name
#endif

#ifndef ENDPROC
#define ENDPROC(_name)							\
	.type _name, %function;						\
	END(_name)
#endif

#ifndef EQUIV_GLOBALSYM
#define EQUIV_GLOBALSYM(_name, _expr)					\
	.global _name;							\
	.equiv _name, _expr
#endif

#ifndef _AC
#define _AC(X,Y)			X
#endif

#ifndef _AT
#define _AT(T,X)			X
#endif

#else /* !__ASSEMBLY__ */
#ifndef ENTRY_ALIGNED
#define ENTRY_ALIGNED(_name, _align)					\
	".global " STRINGIFY(_name) "\n\t"				\
	".align " STRINGIFY(_align) "\n\t";				\
	"" STRINGIFY(_name) ":\n\t"
#endif

#ifndef ENTRY
#define ENTRY(_name)		ENTRY_ALIGNED(_name, 2)
#endif

#ifndef END
#define END(_name)							\
	".size " STRINGIFY(_name) ", . - " STRINGIFY(_name) "\n\t"
#endif

#ifndef ENDPROC
#define ENDPROC(_name)							\
	".type " STRINGIFY(_name) ", %function\n\t"			\
	END(_name)
#endif

#ifndef EQUIV_GLOBALSYM
#define EQUIV_GLOBALSYM(_name, _expr)					\
	asm (								\
		".global " STRINGIFY(_name) "\n\t"			\
		".equiv " STRINGIFY(_name) ", " STRINGIFY(_expr) "\n\t"	\
	)
#endif

#ifndef _AC
#ifndef __AC
#define __AC(X,Y)			(X##Y)
#endif
#define _AC(X,Y)			__AC(X,Y)
#endif

#ifndef _AT
#define _AT(T,X)			((T)(X))
#endif
#endif /* !__ASSEMBLY__ */
#endif /* __UK_ASM_H__ */
