BEGIN { print "/* Auto generated file. Do not edit */" }
/#define __NR_/ {
	name = substr($2,6);
	printf "\n#ifndef HAVE_uk_syscall_%s", name;
	printf "\n#define uk_syscall_e_%s(...) uk_syscall_e_stub(\"%s\")", name, name;
	printf "\n#define uk_syscall_r_%s(...) uk_syscall_r_stub(\"%s\")", name, name;
	printf "\n#endif /* !HAVE_uk_syscall_%s */\n", name;
}
