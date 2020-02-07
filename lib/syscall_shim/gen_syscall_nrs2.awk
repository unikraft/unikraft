BEGIN {
	print "/* Automatically generated file; DO NOT EDIT */"
	print "#ifndef __UK_SYSCALL_NRS_2_H__"
	print "#define __UK_SYSCALL_NRS_2_H__"
}

/#define __NR_/{
	 printf "#define __NR_%s\t\t%s\n", substr($2,6),$3
}

END {
	print "#endif /* __UK_SYSCALL_NRS_2_H__ */"
}
