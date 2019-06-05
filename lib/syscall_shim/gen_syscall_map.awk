BEGIN {print "/* Automatically generated file; DO NOT EDIT */\n"}
/#define __NR_/{
	printf "#define uk_syscall_fn_%s(...) uk_syscall_%s(__VA_ARGS__)\n", $3,substr($2,6)
}
