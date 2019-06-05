BEGIN { print "/* Auto generated file. Do not edit */" }
/#define __NR_/ {
	name = substr($2,6);
	uk_name = "uk_syscall_" name
	printf "\n#ifndef HAVE_%s", uk_name;
	printf "\n#define %s(...) uk_syscall_stub(\"%s\")", uk_name, name;
	printf "\n#endif /* HAVE_%s */\n", uk_name;
}
