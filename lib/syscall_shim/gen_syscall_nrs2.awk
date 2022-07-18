BEGIN {
	print "/* Automatically generated file; DO NOT EDIT */"
	print "#ifndef __LIBSYSCALL_SHIM_SYSCALLS_NRS2_H__"
	print "#define __LIBSYSCALL_SHIM_SYSCALLS_NRS2_H__"
}

/#define __NR_/{
	 printf "\n#define __NR_%s\t\t%s", substr($2,6),$3
}

END {
	print "\n\n#endif /* __LIBSYSCALL_SHIM_SYSCALLS_NRS2_H__ */"
}
