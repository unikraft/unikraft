BEGIN {
	max_args = 6
	print "/* Auto generated file. DO NOT EDIT */\n\n"

	print "#include <uk/syscall.h>"
	print "#include <uk/print.h>\n"

	print "long uk_vsyscall_r(long nr, va_list arg)\n{"
	print "\t(void) arg;\n"
	print "\tswitch (nr) {"
}


/[a-zA-Z0-9]+-[0-9]+/{
	name = $1
	sys_name = "SYS_" name
	uk_syscall_r = "uk_syscall_r_" name
	args_nr = $2 + 0
	printf "\tcase %s:\n", sys_name;
	printf "\t\treturn %s(", uk_syscall_r;
	for (i = 1; i < args_nr; i++)
		printf("va_arg(arg, long), ")
	if (args_nr > 0)
		printf("va_arg(arg, long)")
	printf(");\n")
}

END {
	printf "\tdefault:\n"
	printf "\t\tuk_pr_debug(\"syscall \\\"%%s\\\" is not available\\n\", uk_syscall_name(nr));\n"
	printf "\t\treturn -ENOSYS;\n"
	printf "\t}\n}\n"
}
