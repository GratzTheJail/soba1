.text
.globl foo
foo:
	pushq %rbp
	movq %rsp, %rbp

WHILE:
	cmp $0, %esi
	jz FIM
	decl %esi

	movl $0, %edx
	cvtsi2sd %edx, %xmm0
	movsd %xmm0, (%rdi)

	addq $8, %rdi
	jmp WHILE
FIM:
	leave
	ret


