BEGIN {
	print "/* Automatically generated file; DO NOT EDIT */"
	print ""
	print "#ifndef __UK_STORE_H__"
	print "#error Do not include this header directly"
	print "#endif /* __UK_STORE_H__ */\n"
	print "\nstruct uk_store_entry;\n"

	entries = 0
}

/[a-zA-Z0-9]+/{
	printf "#define __UK_STORE_HAVE_%s 1\n", $1;
	printf "#define __UK_STORE_%s (%d)\n", $1, (entries++);
	printf "extern struct uk_store_entry *uk_store_lib_%s_start;\n", $1;
	printf "extern struct uk_store_entry *uk_store_lib_%s_end;\n\n", $1;
}

END {
	printf "#define __UK_STORE_COUNT (%d)\n", entries;
}
