BEGIN {
	print "/* Automatically generated file; DO NOT EDIT */"
	print "#ifndef __LIBSYSCALL_SHIM_SYSCALL_NRS_H__"
	print "#define __LIBSYSCALL_SHIM_SYSCALL_NRS_H__"
}

/#define __NR_/{
	 printf "\n#define SYS_%s\t\t%s", substr($2,6),$3
}

END {
	print "\n\n#endif /* __LIBSYSCALL_SHIM_SYSCALL_NRS_H__ */"
}
