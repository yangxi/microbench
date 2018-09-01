	.file	"rdtsc.c"
	.text
	.p2align 4,,15
	.globl	kernel
	.type	kernel, @function
kernel:
.LFB43:
	.cfi_startproc
#APP
# 8 "./rdtsc.c" 1
	rdtsc
# 0 "" 2
#NO_APP
	xorpd	%xmm0, %xmm0
	movq	%rax, -16(%rsp)
	xorl	%eax, %eax
	.p2align 4,,10
	.p2align 3
.L2:
	addsd	(%rdi,%rax), %xmm0
	addq	$8, %rax
	cmpq	$8192, %rax
	jne	.L2
#APP
# 8 "./rdtsc.c" 1
	rdtsc
# 0 "" 2
#NO_APP
	cvtsi2sd	%esi, %xmm1
	movq	%rax, -8(%rsp)
	movq	-8(%rsp), %rax
	movq	-16(%rsp), %rcx
	subq	%rcx, %rax
	movq	%rax, (%rdx)
	divsd	%xmm1, %xmm0
	ret
	.cfi_endproc
.LFE43:
	.size	kernel, .-kernel
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC1:
	.string	"total in %lld cycles\n"
.LC2:
	.string	"%lld\n"
	.section	.text.startup,"ax",@progbits
	.p2align 4,,15
	.globl	main
	.type	main, @function
main:
.LFB44:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movl	$8192, %edi
	pushq	%rbx
	.cfi_def_cfa_offset 24
	.cfi_offset 3, -24
	subq	$824, %rsp
	.cfi_def_cfa_offset 848
	call	malloc
	movq	%rax, %rbp
	movabsq	$217020518514230019, %rax
	movq	%rax, 0(%rbp)
#APP
# 8 "./rdtsc.c" 1
	rdtsc
# 0 "" 2
#NO_APP
	xorl	%ebx, %ebx
	movq	%rax, (%rsp)
	.p2align 4,,10
	.p2align 3
.L6:
	movslq	%ebx, %rdx
	movl	$1024, %esi
	movq	%rbp, %rdi
	leaq	16(%rsp,%rdx,8), %rdx
	addl	$1, %ebx
	call	kernel
	cmpl	$10, %ebx
	jne	.L6
#APP
# 8 "./rdtsc.c" 1
	rdtsc
# 0 "" 2
#NO_APP
	movq	%rax, 8(%rsp)
	movq	8(%rsp), %rdx
	leaq	16(%rsp), %rbx
	movq	(%rsp), %rax
	leaq	96(%rsp), %rbp
	movl	$.LC1, %esi
	movl	$1, %edi
	subq	%rax, %rdx
	xorl	%eax, %eax
	call	__printf_chk
	.p2align 4,,10
	.p2align 3
.L7:
	movq	(%rbx), %rdx
	xorl	%eax, %eax
	movl	$.LC2, %esi
	movl	$1, %edi
	addq	$8, %rbx
	call	__printf_chk
	cmpq	%rbp, %rbx
	jne	.L7
	addq	$824, %rsp
	.cfi_def_cfa_offset 24
	xorl	%eax, %eax
	popq	%rbx
	.cfi_def_cfa_offset 16
	popq	%rbp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE44:
	.size	main, .-main
	.ident	"GCC: (Ubuntu/Linaro 4.7.3-2ubuntu1~12.04) 4.7.3"
	.section	.note.GNU-stack,"",@progbits
