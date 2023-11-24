BEGIN {
	max_args = 6
	print "/* Auto generated file. DO NOT EDIT */\n\n"

	print "#include <uk/syscall.h>"
	print "#include <uk/print.h>\n"

	printf "long uk_syscall6(long nr, "
	for (i = 1; i < max_args; i++)
		printf "long arg%d __maybe_unused, ", i
	printf "long arg%d __maybe_unused)\n{\n", max_args
	print "\tlong ret;\n"

	print "\tswitch (nr) {"
}


/[a-zA-Z0-9]+-[0-9]+/{
	name = $1
	sys_name = "SYS_" name
	args_nr = $2 + 0
	printf "#ifdef HAVE_uk_syscall_%s\n", name;
	printf "\tcase %s:\n", sys_name;
	printf "\t\tret = uk_syscall_e_%s(", name;
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
	printf "\t\terrno = -ENOSYS;\n"
	printf "\t\tret = -1;\n"
	printf "\t}\n"
	printf "\treturn ret;\n"
	printf "}\n"
	printf "\n"
}
