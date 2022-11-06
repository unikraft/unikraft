
.global call_binary_fork
.global call_fork
.data
	.extern return_addr

aux:
	push %rax
	movq 8(%rsp), %rax
	add $2, %rax
	movq %rax, return_addr
	pop %rax
	ret

aux2:
	push %rax
	movq 8(%rsp), %rax
	add $5, %rax
	movq %rax, return_addr
	pop %rax
	ret

call_binary_fork:
	mov $57, %rax
	call aux
	syscall
	ret

call_fork:
	call aux2
	call uk_syscall_r_fork
	ret
