BEGIN {print "/* Automatically generated file; DO NOT EDIT */"}
/#define __NR_/{
	 printf "\n#define SYS_%s\t\t%s", substr($2,6),$3
}
