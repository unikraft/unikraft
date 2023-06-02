BEGIN {
	print "/* Automatically generated file; DO NOT EDIT */"
	print ""
	print "static struct uk_store_entry *static_entries[] = {"
}

/[a-zA-Z0-9]+/{
	printf "\t\t(struct uk_store_entry *) &uk_store_lib_%s_start,\n", $1;
	printf "\t\t(struct uk_store_entry *) &uk_store_lib_%s_end,\n", $1;
}

END {
	print "\t};"
	print ""
}
