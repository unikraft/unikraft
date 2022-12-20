BEGIN {
	print "/* Automatically generated file; DO NOT EDIT */"
	print ""
	print "SECTIONS"
	print ""
	print "{"
}

/[a-zA-Z0-9]+/{
	printf "\t.uk_store_lib_%s : {\n", $1;
	printf "\t\tPROVIDE(uk_store_lib_%s_start = .);\n", $1;
	printf "\t\tKEEP(*(.uk_store_lib_%s));\n", $1;
	printf "\t\tKEEP(*(.uk_store_lib_%s.*));\n", $1;
	printf "\t\tPROVIDE(uk_store_lib_%s_end = .);\n", $1;
	print  "\t}"
	print  ""
}

END {
	print "}"
	print "INSERT AFTER .rodata"
	print ""
}
