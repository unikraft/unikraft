/[a-zA-Z0-9]+-[0-9]+e?/{
	name = $1;
	printf "uk_syscall_r_%s\n", name;
	printf "uk_syscall_e_%s\n", name;
	printf "uk_syscall_do_%s\n", name;
	if (substr($0, length($0)) == "e") {
		printf "uk_syscall_r_e_%s\n", name;
		printf "uk_syscall_e_e_%s\n", name;
		printf "uk_syscall_do_e_%s\n", name;
	}
}
