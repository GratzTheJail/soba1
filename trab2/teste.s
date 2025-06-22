.text
.globl foo
foo:
	pushq %rbp
	movq %rsp, %rbp	
	subq $32, %rsp

	movl $-123, -4(%rbp)

	movl -4(%rbp), %ecx
	movl %ecx, -8(%rbp)
	
	movl -8(%rbp), %eax

	leave
	ret


