SECTIONS
{
	.uk_tracepoints_list : {
		uk_tracepoints_start = .;
		KEEP (*(.uk_tracepoints_list));
		uk_tracepoints_end = .;
	}
	.uk_trace_keyvals : {
		KEEP (*(.uk_trace_keyvals));
	}
}

/* We do not want these section to be in the final image, they are
 * needed only offline. Solution is obvious - ask the build system to
 * strip them together with debug symbols.
 *
 * However, we need to put them at them somewhere close to the end of
 * the elf, otherwise strip operation will make a hole in the address
 * space. More then that, they have to reside _after_ the '_end'
 * symbol.
 *
 * The trick is to allocate the sections after the '.comment'
 * section. The other way would have been allocating a dummy section
 * '.cut_here', but linker drops it if there is nothing in it.
 */
INSERT AFTER .comment;
