BEGIN {
	max_args = 6
	print "/* Auto generated file. DO NOT EDIT */\n\n"

	print "#include <uk/syscall.h>"
	print "#include <uk/print.h>\n"

	print "long uk_vsyscall_r(long nr, va_list arg)\n{"
	print "\t(void) arg;\n"

	printf "\t__maybe_unused long "
	for (i = 1; i < max_args; i++)
		printf "a%d, ", i
	printf "a%d;\n", max_args
	print "\tswitch (nr) {"
}


/[a-zA-Z0-9]+-[0-9]+/{
	name = $1
	sys_name = "SYS_" name
	uk_syscall_r = "uk_syscall_r_" name
	args_nr = $2 + 0
	printf "#ifdef HAVE_uk_syscall_%s\n", name;
	printf "\tcase %s:\n", sys_name;
	for (i = 1; i <= args_nr; i++)
		printf "\t\ta%s = va_arg(arg, long);\n", i;
	printf "\t\treturn %s(", uk_syscall_r;
	for (i = 1; i < args_nr; i++)
		printf "a%d, ", i;
	if (args_nr > 0)
		printf "a%d", args_nr;
	printf(");\n")
	printf "#endif /* HAVE_uk_syscall_%s */\n\n", name;
}

END {
	printf "\tdefault:\n"
	printf "\t\tuk_pr_debug(\"syscall \\\"%%s\\\" is not available\\n\", uk_syscall_name(nr));\n"
	printf "\t\treturn -ENOSYS;\n"
	printf "\t}\n}\n"
	printf "\n"

	printf "long uk_syscall_r(long nr, ...)\n"
	printf "{\n"
	printf "\tlong ret;\n"
	printf "\tva_list ap;\n"
	printf "\n"
	printf "\tva_start(ap, nr);\n"
	printf "\tret = uk_vsyscall_r(nr, ap);\n"
	printf "\tva_end(ap);\n"
	printf "\treturn ret;\n"
	printf "}\n"
}
