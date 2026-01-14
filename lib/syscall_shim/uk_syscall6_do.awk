BEGIN {
	max_args = 6
	print "/* Auto generated file. DO NOT EDIT */\n\n"

	print "#include <uk/print.h>"
	print "#include <uk/syscall.h>"
	print "#include <uk/bits/syscall_linuxabi.h>\n"

	print "UK_SYSCALL_EXECENV_PROLOGUE_DEFINE(uk_syscall6_do, uk_syscall6_do_e,"
	print "\t\t\t\t14, long, nr, long, arg1, long, arg2, long, arg3, long, arg4, long, arg5, long, arg6)";
	print "\nlong __used uk_syscall6_do_e(struct ukarch_execenv *execenv)"
	print "{"
	print "\tlong ret;"
	print "\n\tswitch (_UK_EXECENV_REGS_GET(execenv, __syscall_rsyscall)) {"
}

/[a-zA-Z0-9]+-[0-9]+/{
	name = $1
	sys_name = "SYS_" name
	uk_syscall_do = "uk_syscall_do_" name
	uk_syscall_do_e = "uk_syscall_do_e_" name
	args_nr = $2 + 0
	printf "\n#ifdef HAVE_uk_syscall_%s\n", name;
	printf "\tcase %s:\n", sys_name;
	printf "\n#ifdef HAVE_uk_syscall_e_%s\n", name;
	printf "\t\tret = %s((long)execenv);\n", uk_syscall_do_e;
	printf "#else /* !HAVE_uk_syscall_e_%s */\n", name;
	printf "\t\tret = %s(\n\t\t\t\t\t", uk_syscall_do;
	for (i = 0; i < args_nr - 1; i++)
		printf("_UK_EXECENV_REGS_GET(execenv, __syscall_rarg%d), ", i)
	if (args_nr > 0)
		printf("_UK_EXECENV_REGS_GET(execenv, __syscall_rarg%d)", args_nr - 1)
	printf(");\n")
	printf "\n#endif /* !HAVE_uk_syscall_e_%s */\n\n", name;
	printf "\t\tbreak;\n"
	printf "\n#endif /* HAVE_uk_syscall_%s */\n", name;
}

END {
	printf "\tdefault:\n"
	printf "\t\tuk_pr_debug(\"syscall \\\"%%s\\\" is not available\\n\", uk_syscall_name(_UK_EXECENV_REGS_GET(execenv, __syscall_rsyscall)));\n"
	printf "\t\tret = -ENOSYS;\n"
	printf "\t}\n"
	printf "\treturn ret;\n"
	printf "}\n"
	printf "\n"
}
