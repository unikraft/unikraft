/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2026, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/arch.h>
#include <uk/config.h>
#include <uk/essentials.h>

SECTIONS {
	/**
	 * The sh_addralign reflects the alignment of the input sections
	 * (i.e., our variables), not the linker script's section alignment
	 * constraint.
	 * Therefore, to work around not being able to control that section
	 * header attribute, export the alignment as a symbol so that we can
	 * tell the alignment from the image parsing instead of burdening the
	 * build system with providing the correct alignment argument to the
	 * postprocessing script.
	 */
	_uk_pcpuvar_align = UK_ARCH_CACHE_LINE_SIZE;

	.uk_pcpuvar :
	{
		. = ALIGN(UK_ARCH_CACHE_LINE_SIZE);
		_uk_pcpuvar_base = .;

		_uk_pcpuvar_tmpl_start = .;
		KEEP(*(.uk_pcpuvar*))
		. = ALIGN(UK_ARCH_CACHE_LINE_SIZE);
		_uk_pcpuvar_tmpl_end = .;

		/**
		 * ABSOLUTE() forces the linker to record _uk_pcpuvar_tmpl_size
		 * in the symbol table with section index SHN_ABS rather than
		 * as an offset relative to the enclosing section.
		 * A plain symbol difference such as
		 * (_uk_pcpuvar_tmpl_end - _uk_pcpuvar_tmpl_start) would be
		 * section-relative, causing the toolchain to emit a load-time
		 * relocation entry wherever the symbol is referenced. That
		 * relocation must be resolved before the value is usable,
		 * making it unsafe to reference in early assembly that runs
		 * before self-relocation has completed.
		 * With SHN_ABS the value is a pure integer baked into the
		 * symbol table at link time: no relocation entry is emitted
		 * for any reference to it, regardless of architecture or
		 * access pattern.
		 */
		_uk_pcpuvar_tmpl_size = ABSOLUTE(_uk_pcpuvar_tmpl_end -
						 _uk_pcpuvar_tmpl_start);

		/* Explicit zero padding for remaining CPU copies */
		FILL(0x00);
		. = _uk_pcpuvar_tmpl_start +
		    ALIGN_UP(_uk_pcpuvar_tmpl_end - _uk_pcpuvar_tmpl_start,
			     UK_ARCH_CACHE_LINE_SIZE) *
			     CONFIG_UKPLAT_CPU_MAXCOUNT;

		_uk_pcpuvar_end = .;
	}
}

INSERT AFTER .data
