BEGIN {print "/* Automatically generated file; DO NOT EDIT */"}
/[a-zA-Z0-9]+-[0-9]+/{
	printf "\n#define HAVE_uk_syscall_%s t", $1;
	printf "\nUK_SYSCALL_PROTO(%s, %s);\n", $2, $1;
}
