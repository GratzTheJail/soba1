.text
.globl foo
foo:
	pushq %rbp
	movq %rsp, %rbp	
	subq $32, %rsp

	movl -20(%rbp), %ecx
        imull -8(%rbp), %ecx
	movl %ecx, -4(%rbp)

	movl -4(%rbp), %eax

	leave
	ret

