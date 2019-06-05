BEGIN {
	max_args = 6
	print "/* Auto generated file. DO NOT EDIT */\n\n"

	print "#include <uk/syscall.h>"
	print "#include <uk/print.h>\n"

	printf "static inline long __syscall_dynamic(long nr, "
	for (i = 1; i < max_args; i++)
		printf "long arg%d, ",i
	printf "long arg%d)\n{\n", max_args

	for (i = 1; i <= max_args; i++)
		printf "\t(void) arg%d;\n", i

	print "\n\tswitch (nr) {"
}


/[a-zA-Z0-9]+-[0-9]+/{
	name = $1
	sys_name = "SYS_" name
	uk_name = "uk_syscall_" name
	args_nr = $2 + 0
	printf "\tcase %s:\n", sys_name;
	printf "\t\treturn %s(", uk_name;
	for (i = 1; i < args_nr; i++)
		printf("arg%d, ", i)
	if (args_nr > 0)
		printf("arg%d", args_nr)
	printf(");\n")
}

END {
	printf "\tdefault:\n"
	printf "\t\tuk_pr_debug(\"syscall nr %%ld is not implemented\", nr);\n"
	printf "\t\terrno = -ENOSYS;\n"
	printf "\t\treturn -1;\n"
	printf "\t}\n}\n"
}
