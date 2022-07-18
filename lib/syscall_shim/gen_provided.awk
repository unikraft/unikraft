BEGIN {
	print "/* Automatically generated file; DO NOT EDIT */"
	print "#ifndef __LIBSYSCALL_SHIM_PROVIDED_SYSCALLS_H__"
	print "#define __LIBSYSCALL_SHIM_PROVIDED_SYSCALLS_H__"
	print "\n#include <uk/bits/syscall_nrs.h>"
}

/[a-zA-Z0-9]+-[0-9]+/{
	printf "\n#ifndef SYS_%s\n", $1;
	printf "#warning Ignoring system call '%s': No system call number available\n", $1
	printf "#else\n";
	printf "#define HAVE_uk_syscall_%s t\n", $1;
	printf "UK_SYSCALL_E_PROTO(%s, %s);\n", $2, $1;
	printf "UK_SYSCALL_R_PROTO(%s, %s);\n", $2, $1;
	printf "#endif /* !SYS_%s */\n", $1;
}

END {
	print "\n#endif /* __LIBSYSCALL_SHIM_PROVIDED_SYSCALLS_H__ */"
}
