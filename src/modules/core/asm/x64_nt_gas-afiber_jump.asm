.file	"x64_nt_gas-afiber_jump.asm"
.text
.p2align 4,,15
.globl	afiber_jump
.def	afiber_jump
	.scl	2
    .type	32
.endef
.seh_proc afiber_jump
afiber_jump:
.seh_endprologue






/* restore RSP (pointing to context-data) from RDX */
    movq  %rdx, %rsp

#if !defined(BOOST_USE_TSX)
    /* restore XMM storage */
    movaps  0x0(%rsp), %xmm6
    movaps  0x10(%rsp), %xmm7
    movaps  0x20(%rsp), %xmm8
    movaps  0x30(%rsp), %xmm9
    movaps  0x40(%rsp), %xmm10
    movaps  0x50(%rsp), %xmm11
    movaps  0x60(%rsp), %xmm12
    movaps  0x70(%rsp), %xmm13
    movaps  0x80(%rsp), %xmm14
    movaps  0x90(%rsp), %xmm15
 	ldmxcsr 0xa0(%rsp) /* restore MMX control- and status-word */
 	fldcw   0xa4(%rsp) /* restore x87 control-word */
#endif

    /* load NT_TIB */
    movq  %gs:(0x30), %r10
    /* restore fiber local storage */
    movq  0xb0(%rsp), %rax
    movq  %rax, 0x20(%r10)
    /* restore current deallocation stack */
    movq  0xb8(%rsp), %rax
    movq  %rax, 0x1478(%r10)
    /* restore current stack limit */
    movq  0xc0(%rsp), %rax
    movq  %rax, 0x10(%r10)
    /* restore current stack base */
    movq  0xc8(%rsp), %rax
    movq  %rax, 0x08(%r10)

    movq  0xd0(%rsp),  %r12  /* restore R12 */
    movq  0xd8(%rsp),  %r13  /* restore R13 */
    movq  0xe0(%rsp),  %r14  /* restore R14 */
    movq  0xe8(%rsp),  %r15  /* restore R15 */
    movq  0xf0(%rsp),  %rdi  /* restore RDI */
    movq  0xf8(%rsp),  %rsi  /* restore RSI */
    movq  0x100(%rsp), %rbx  /* restore RBX */
    movq  0x108(%rsp), %rbp  /* restore RBP */


    movq  0x110(%rsp), %rax  /* restore hidden address of transport_t */

    leaq  0x118(%rsp), %rsp /* prepare stack */

    /* restore return-address */
    popq  %r10

    /* transport_t returned in RAX */
    /* return parent fcontext_t */
    movq  %r9, 0x0(%rax)
    /* return data */
    movq  %r8, 0x8(%rax)

    /* transport_t as 1.arg of context-function */
    movq  %rax, %rcx

    /* indirect jump to context */
    jmp  *%r10


.seh_endproc

.section .drectve
.ascii " -export:\"afiber_jump\""