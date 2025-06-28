.text
.globl main
main:
	pushq %rbp
	movq %rsp, %rbp	
	subq $32, %rsp

	movl $-5, -8(%rbp)
	cmpl $0, -8(%rbp)
	jbe teste
	movl $0, %eax
	
	leave
	ret

teste:
	movl $1, %eax

	leave
	ret

