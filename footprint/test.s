	.file	"test.c"
	.text
	.globl	prefetch
	.type	prefetch, @function
prefetch:
.LFB2:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
	prefetchnta	(%rax)
	movq	-8(%rbp), %rax
	prefetchnta	(%rax)
	movq	-8(%rbp), %rax
	prefetcht2	(%rax)
	movq	-8(%rbp), %rax
	prefetcht2	(%rax)
	movq	-8(%rbp), %rax
	prefetcht1	(%rax)
	movq	-8(%rbp), %rax
	prefetcht0	(%rax)
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE2:
	.size	prefetch, .-prefetch
	.ident	"GCC: (Ubuntu 4.9.2-10ubuntu13) 4.9.2"
	.section	.note.GNU-stack,"",@progbits
