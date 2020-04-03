BEGIN {
	print "/* Auto generated file. Do not edit */"
	print "\n#include <errno.h>"
	print "\n#include <uk/syscall.h>"
	print "\n#include <uk/print.h>"
	print "\n#include <uk/essentials.h>"
}
/#define __NR_/ {
	name = substr($2,6);
	printf "\n#ifndef HAVE_uk_syscall_%s", name;
	printf "\nlong __weak %s(void)", name;
	printf "\n{";
	printf "\n\treturn uk_syscall_e_stub(\"%s\");", name;
	printf "\n}";
	printf "\n#endif /* !HAVE_uk_syscall_%s */\n", name;
}
