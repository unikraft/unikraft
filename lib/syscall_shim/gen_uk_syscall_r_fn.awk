BEGIN {
	print "/* Auto generated file. DO NOT EDIT */\n"

	print "#include <uk/syscall.h>"
	print "#include <uk/print.h>"
	print "#include <stdlib.h>\n"

	print "long (*uk_syscall_r_fn(long nr))(void)\n{"
	print "\tswitch (nr) {"
}

/[a-zA-Z0-9]+-[0-9]+/{
	name = $1
	sys_name = "SYS_" name
	uk_syscall_r = "uk_syscall_r_" name
	printf "\tcase %s:\n", sys_name;
	printf "\t\treturn (long (*)(void)) %s;\n", uk_syscall_r;
}

END {
	print "\tdefault:"
	print "\t\treturn NULL;"
	print "\t}\n}"
}
