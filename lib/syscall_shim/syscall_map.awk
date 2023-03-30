BEGIN {
	max_args = 6
	print "/* Automatically generated file; DO NOT EDIT */\n"
	print "#include <uk/bits/syscall_provided.h>\n"
	print "#define _uk_syscall_e_enosys() \\"
	print "\t({ errno = ENOSYS; (-1); })"
	print "#define _uk_syscall_r_enosys() \\"
	print "\t({ (-ENOSYS); })"
}
/#define __NR_/{
	printf "\n"
	printf "#ifdef HAVE_uk_syscall_%s\n", substr($2,6);
	printf "#define uk_syscall_fn%d(...) \\\n\tuk_syscall_e_%s(__VA_ARGS__)\n", $3, substr($2,6)
	for (i = 0; i <= max_args; i++)
	{
		printf "#define uk_syscall%d_fn%d(", i, $3
		for (j = 1; j <= i; j++)
		{
			if (j > 1)
				printf ", ";
			printf "arg%d", j;
		}
		printf ") \\\n\tuk_syscall_e%d_%s(", i, substr($2,6)
		for (j = 1; j <= i; j++)
		{
			if (j > 1)
				printf ", ";
			printf "arg%d", j;
		}
		printf ")\n";
	}
	printf "#define uk_syscall_r_fn%d(...) \\\n\tuk_syscall_r_%s(__VA_ARGS__)\n", $3, substr($2,6)
	for (i = 0; i <= max_args; i++)
	{
		printf "#define uk_syscall_r%d_fn%d(", i, $3
		for (j = 1; j <= i; j++)
		{
			if (j > 1)
				printf ", ";
			printf "arg%d", j;
		}
		printf ") \\\n\tuk_syscall_r%d_%s(", i, substr($2,6)
		for (j = 1; j <= i; j++)
		{
			if (j > 1)
				printf ", ";
			printf "arg%d", j;
		}
		printf ")\n";
	}
	printf "#else /* !HAVE_uk_syscall_%s */\n", substr($2,6);

	printf "#define uk_syscall_fn%d(...) _uk_syscall_e_enosys()\n", $3
	for (i = 0; i <= max_args; i++)
	{
		printf "#define uk_syscall%d_fn%d(", i, $3
		for (j = 1; j <= i; j++)
		{
			if (j > 1)
				printf ", ";
			printf "arg%d", j;
		}
		printf ") _uk_syscall_e_enosys()\n";
	}
	printf "#define uk_syscall_r_fn%d(...) _uk_syscall_r_enosys()\n", $3
	for (i = 0; i <= max_args; i++)
	{
		printf "#define uk_syscall_r%d_fn%d(", i, $3
		for (j = 1; j <= i; j++)
		{
			if (j > 1)
				printf ", ";
			printf "arg%d", j;
		}
		printf ") _uk_syscall_r_enosys()\n";
	}
	printf "#endif /* !HAVE_uk_syscall_%s */\n", substr($2,6);
}
