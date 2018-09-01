	.file	"threadrdtsc.c"
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC0:
	.string	"profiler is ready"
	.text
	.p2align 4,,15
	.globl	profiler
	.type	profiler, @function
profiler:
.LFB64:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rdi, %rbp
	movl	$8192, %edi
	pushq	%rbx
	.cfi_def_cfa_offset 24
	.cfi_offset 3, -24
	subq	$8, %rsp
	.cfi_def_cfa_offset 32
	call	malloc
	movq	%rax, %rdx
	movq	%rax, tag(%rip)
	movl	$1024, %ecx
	xorl	%eax, %eax
	movq	%rdx, %rdi
	rep stosq
	movl	$.LC0, %edi
	call	puts
	movq	tag(%rip), %rdi
#APP
# 32 "threadrdtsc.c" 1
	movq $0, %rax
	movq $0, %rbx
	S:
	incq %rax
	movl (%rbp, %rbx, 4), %rcx
	testl %rcx, %rcx
	jeq S
	movq %rax, (%rdi, %rbx, 8)
	incq %rbx
	jmp S:
	
# 0 "" 2
#NO_APP
	xorl	%eax, %eax
	xorl	%esi, %esi
	xorl	%edx, %edx
	.p2align 4,,10
	.p2align 3
.L9:
	leaq	0(%rbp,%rdx,4), %rcx
	addq	$1, %rax
	movl	(%rcx), %ecx
	testl	%ecx, %ecx
	je	.L9
.L12:
	addl	$1, %esi
	movq	%rax, (%rdi,%rdx,8)
	addq	$1, %rax
	movslq	%esi, %rdx
	leaq	0(%rbp,%rdx,4), %rcx
	movl	(%rcx), %ecx
	testl	%ecx, %ecx
	je	.L9
	jmp	.L12
	.cfi_endproc
.LFE64:
	.size	profiler, .-profiler
	.p2align 4,,15
	.globl	rdtsc
	.type	rdtsc, @function
rdtsc:
.LFB63:
	.cfi_startproc
#APP
# 20 "threadrdtsc.c" 1
	rdtscp
# 0 "" 2
#NO_APP
	salq	$32, %rdx
	movl	%eax, %eax
	orq	%rax, %rdx
	movq	%rdx, %rax
	ret
	.cfi_endproc
.LFE63:
	.size	rdtsc, .-rdtsc
	.section	.rodata.str1.1
.LC1:
	.string	"create profiling thread"
.LC2:
	.string	"%d: %lld    %lld\n"
	.section	.text.startup,"ax",@progbits
	.p2align 4,,15
	.globl	main
	.type	main, @function
main:
.LFB65:
	.cfi_startproc
	pushq	%rbx
	.cfi_def_cfa_offset 16
	.cfi_offset 3, -16
	movl	$4096, %edi
	subq	$8272, %rsp
	.cfi_def_cfa_offset 8288
	movq	%fs:40, %rax
	movq	%rax, 8264(%rsp)
	xorl	%eax, %eax
	call	malloc
	xorl	%edx, %edx
	.p2align 4,,10
	.p2align 3
.L15:
	leaq	(%rax,%rdx), %rcx
	addq	$4, %rdx
	movq	%rax, %rbx
	cmpq	$4096, %rdx
	movl	$0, (%rcx)
	jne	.L15
	movl	$.LC1, %edi
	call	puts
	leaq	8208(%rsp), %rdi
	call	pthread_attr_init
	leaq	8208(%rsp), %rsi
	movq	%rbx, %rcx
	movl	$profiler, %edx
	movq	%rsp, %rdi
	call	pthread_create
	movl	$2, %edi
	xorl	%eax, %eax
	call	sleep
	leaq	16(%rsp), %rsi
	movq	%rsi, %rdi
	addq	$8192, %rdi
	.p2align 4,,10
	.p2align 3
.L16:
#APP
# 20 "threadrdtsc.c" 1
	rdtscp
# 0 "" 2
#NO_APP
	salq	$32, %rdx
	movl	%eax, %eax
	orq	%rax, %rdx
	movq	%rdx, 8(%rsp)
	movq	8(%rsp), %rax
	movl	$1, (%rbx)
	addq	$4, %rbx
	movq	%rax, (%rsi)
	addq	$8, %rsi
	cmpq	%rdi, %rsi
	jne	.L16
	movl	$2, %edi
	xorl	%eax, %eax
	xorl	%ebx, %ebx
	call	sleep
	.p2align 4,,10
	.p2align 3
.L17:
	movq	tag(%rip), %rax
	movq	16(%rsp,%rbx,8), %rcx
	movl	%ebx, %edx
	movl	$.LC2, %esi
	movl	$1, %edi
	movq	(%rax,%rbx,8), %r8
	xorl	%eax, %eax
	addq	$1, %rbx
	call	__printf_chk
	cmpq	$20, %rbx
	jne	.L17
	movq	8264(%rsp), %rdx
	xorq	%fs:40, %rdx
	jne	.L23
	addq	$8272, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 16
	popq	%rbx
	.cfi_def_cfa_offset 8
	ret
.L23:
	.cfi_restore_state
	call	__stack_chk_fail
	.cfi_endproc
.LFE65:
	.size	main, .-main
	.comm	tag,8,8
	.ident	"GCC: (Ubuntu/Linaro 4.7.3-2ubuntu1~12.04) 4.7.3"
	.section	.note.GNU-stack,"",@progbits
