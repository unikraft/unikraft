BEGIN {
	print "/* Automatically generated file; DO NOT EDIT */"
	print "#ifndef __LIBSYSCALL_SHIM_PROVIDED_SYSCALLS_H__"
	print "#define __LIBSYSCALL_SHIM_PROVIDED_SYSCALLS_H__"
	print "\n#include <uk/bits/syscall_nrs.h>"
}

/[a-zA-Z0-9]+-[0-9]+u?/{
	# check if the syscall is not defined
	printf "\n#ifndef SYS_%s\n", $1;
	# if a LEGACY_<syscall_name> symbol is defined, the syscall is not required
	# and we can continue the build process
	printf "#ifdef LEGACY_SYS_%s\n", $1;
	printf "#ifdef CONFIG_LIBSYSCALL_SHIM_LEGACY_VERBOSE\n"
	printf "#warning Ignoring legacy system call '%s': No system call number available\n", $1
	printf "#endif /* LIBSYSCALL_SHIM_LEGACY_VERBOSE */\n"
	# if the LEGACY symbol is not defined for this syscall, it means that the
	# syscall is required and we can't continue without a definition
	printf "#else\n";
	printf "#error Failed to map system call '%s': No system call number available\n", $1
	printf "#endif  /* LEGACY_SYS_%s */\n", $1
	printf "#else\n";
	if (substr($0, length($0)) == "u") {
		printf "#define HAVE_uk_syscall_e_%s t\n", $1;
		printf "UK_SYSCALL_E_PROTO(1, e_%s);\n", $1;
		printf "UK_SYSCALL_R_PROTO(1, e_%s);\n", $1;
		printf "#define HAVE_uk_syscall_%s t\n", $1;
		printf "UK_SYSCALL_E_PROTO(%s, %s);\n", $2 + 0, $1;
		printf "UK_SYSCALL_R_PROTO(%s, %s);\n", $2 + 0, $1;
	} else {
		printf "#define HAVE_uk_syscall_%s t\n", $1;
		printf "UK_SYSCALL_E_PROTO(%s, %s);\n", $2, $1;
		printf "UK_SYSCALL_R_PROTO(%s, %s);\n", $2, $1;
	}
	printf "#endif /* !SYS_%s */\n", $1;
}

END {
	print "\n#endif /* __LIBSYSCALL_SHIM_PROVIDED_SYSCALLS_H__ */"
}
