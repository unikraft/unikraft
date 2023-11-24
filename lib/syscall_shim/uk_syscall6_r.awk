BEGIN {
	max_args = 6
	print "/* Auto generated file. DO NOT EDIT */\n\n"

	print "#include <uk/print.h>"
	print "#include <uk/syscall.h>"
	print "#include \"arch/regmap_linuxabi.h\"\n"

	print "UK_SYSCALL_USC_PROLOGUE_DEFINE(uk_syscall6_r, uk_syscall6_r_u,"
	print "\t\t\t\t14, long, nr, long, arg1, long, arg2, long, arg3, long, arg4, long, arg5, long, arg6)";
	print "\nlong __used uk_syscall6_r_u(struct uk_syscall_ctx *usc)"
	print "{"
	print "\tlong ret;"
	print "\n\tswitch (usc->regs.rsyscall) {"
}

/[a-zA-Z0-9]+-[0-9]+/{
	name = $1
	sys_name = "SYS_" name
	uk_syscall_r = "uk_syscall_r_" name
	uk_syscall_r_u = "uk_syscall_r_u_" name
	args_nr = $2 + 0
	printf "\n#ifdef HAVE_uk_syscall_%s\n", name;
	printf "\tcase %s:\n", sys_name;
	printf "\n#ifdef HAVE_uk_syscall_u_%s\n", name;
	printf "\t\tret = %s((long)usc);\n", uk_syscall_r_u;
	printf "#else /* !HAVE_uk_syscall_u_%s */\n", name;
	printf "\t\tret = %s(\n\t\t\t\t\t", uk_syscall_r;
	for (i = 0; i < args_nr - 1; i++)
		printf("usc->regs.rarg%d, ", i)
	if (args_nr > 0)
		printf("usc->regs.rarg%d", args_nr - 1)
	printf(");\n")
	printf "\n#endif /* !HAVE_uk_syscall_u_%s */\n\n", name;
	printf "\t\tbreak;\n"
	printf "\n#endif /* HAVE_uk_syscall_%s */\n", name;
}

END {
	printf "\tdefault:\n"
	printf "\t\tuk_pr_debug(\"syscall \\\"%%s\\\" is not available\\n\", uk_syscall_name(usc->regs.rsyscall));\n"
	printf "\t\tret = -ENOSYS;\n"
	printf "\t}\n"
	printf "\treturn ret;\n"
	printf "}\n"
	printf "\n"
}
