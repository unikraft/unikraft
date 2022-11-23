BEGIN {
	max_args = 6
	print "/* Auto generated file. DO NOT EDIT */\n\n"

	print "#include <uk/syscall.h>"
	print "#include <uk/print.h>\n"

	printf "long uk_syscall6_r(long nr, "
	for (i = 1; i < max_args; i++)
		printf "long arg%d __maybe_unused, ", i
	printf "long arg%d __maybe_unused)\n{\n", max_args
	print "\tlong ret;\n"

	print "\t__UK_SYSCALL_RETADDR_ENTRY();"
	print "\tswitch (nr) {"
}


/[a-zA-Z0-9]+-[0-9]+/{
	name = $1
	sys_name = "SYS_" name
	uk_syscall_r = "uk_syscall_r_" name
	args_nr = $2 + 0
	printf "#ifdef HAVE_uk_syscall_%s\n", name;
	printf "\tcase %s:\n", sys_name;
	printf "\t\tret = %s(", uk_syscall_r;
	for (i = 1; i < args_nr; i++)
		printf("arg%d, ", i)
	if (args_nr > 0)
		printf("arg%d", args_nr)
	printf(");\n")
	printf "\t\tbreak;\n"
	printf "#endif /* HAVE_uk_syscall_%s */\n\n", name;
}

END {
	printf "\tdefault:\n"
	printf "\t\tuk_pr_debug(\"syscall \\\"%%s\\\" is not available\\n\", uk_syscall_name(nr));\n"
	printf "\t\tret = -ENOSYS;\n"
	printf "\t}\n"
	printf "\t__UK_SYSCALL_RETADDR_CLEAR();\n"
	printf "\treturn ret;\n"
	printf "}\n"
	printf "\n"
}
