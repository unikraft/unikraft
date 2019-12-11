BEGIN {
	print "/* Auto generated file. DO NOT EDIT */\n"

	print "#include <stddef.h>\n"
	print "#include <uk/syscall.h>\n"

	print "const char *uk_syscall_name_p(long nr)\n{"
	print "\tswitch (nr) {"
}

/[a-zA-Z0-9]+-[0-9]+/{
	printf "\tcase SYS_%s:\n", $1
	printf "\t\treturn \"%s\";\n", $1
}

END {
	print "\tdefault:"
	print "\t\treturn NULL;"
	print "\t}\n}"
}
