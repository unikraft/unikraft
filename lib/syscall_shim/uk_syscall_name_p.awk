BEGIN {
	print "/* Auto generated file. DO NOT EDIT */\n"

	print "#include <stddef.h>\n"
	print "#include <uk/syscall.h>\n"

	print "const char *uk_syscall_name_p(long nr)\n{"
	print "\tswitch (nr) {"
}

/#define __NR_/{
	name = substr($2,6)
	printf "#ifdef HAVE_uk_syscall_%s\n", name;
	printf "\tcase SYS_%s:\n", name
	printf "\t\treturn \"%s\";\n", name
	printf "#endif /* HAVE_uk_syscall_%s */\n\n", name;
}

END {
	print "\tdefault:"
	print "\t\treturn NULL;"
	print "\t}\n}"
}
