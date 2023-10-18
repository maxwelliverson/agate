.file	"x64_nt_gas-afiber_fork.asm"
.text
.p2align 4,,15
.globl	afiber_fork
.def	afiber_fork
	.scl	2
    .type	32
.endef
.seh_proc afiber_fork
afiber_fork:
.seh_endprologue

    /*
        rcx: agt_fiber_transfer_t* hiddenAddress
        rdx: agt_fiber_proc_t      proc
        r8:  agt_fiber_param_t     param
        r9:  agt_fiber_flags_t     flags
    */


    leaq 0x8(%rsp), %r10  /* load the caller's stack address to %r10. */


    /* the following is the preferred way of accessing the TEB, but I don't understand whether or not this is still relevant in the modern x64 era... */
    /*
        movq %gs:(0x30), %r11
        movq 0x20(%r11), %r11
    */

    movq %gs:(0x20), %r11 /* load current fiber */

    andq $-64, %rsp       /* align stack to 64 bytes */

    movb %r9l, %al        /* move flags to rax register (only need to move the low byte) */

    testb $1              /* if (flags & AGT_FIBER_SAVE_EXTENDED_STATE) != 0 (implicit param %al) */
    jz .L_forkPostXSave   /*   goto forkPostXSave */

    pushq            %rdx   /* copy proc to stack (rdx is needed as an implicit parameter for any of the instructions in the xsave family) */
    subq 0x18(%r11), %rsp   /* subtract fromFiber->saveRegionSize from stack pointer */
    movq 0x10(%r11), %rax   /* load storeStateFlags to rax */
    movq %rax,       %rdx   /* copy storeStateFlags to rdx */
    shrq $32,        %rdx   /* shift the high 32 bits of the mask down to fit into edx */
    xsaveopt64   0x8(%rsp)  /* save extended state to data (implicit params %eax and %edx; offset of 8 bytes needed because of rdx having been pushed to the stack) */
    popq             %rdx   /* restore proc to rdx */

.L_forkPostXSave:

    subq $0x80,  %rsp       /* allocate 128 bytes to stack */

    movq 0x00(%r11), %rax   /* load fiber->privateData to %rax */
    movq %rsp,  0x00(%r11)  /* write data to fiber->privateData */
    movq %rax,  0x00(%rsp)  /* write fiber->privateData to data->prevData */

    mov  %r9d,  0x50(%rsp)  /* store flags to data->saveFlags */

    movq %rbx,  0x10(%rsp)  /* store %rbx to data->rbx */
    movq %rbp,  0x18(%rsp)  /* store %rbp to data->rbp */
    movq %rdi,  0x20(%rsp)  /* store %rdi to data->rdi */
    movq %rsi,  0x28(%rsp)  /* store %rsi to data->rsi */
    movq %r12,  0x30(%rsp)  /* store %r12 to data->r12 */
    movq %r13,  0x38(%rsp)  /* store %r13 to data->r13 */
    movq %r14,  0x40(%rsp)  /* store %r14 to data->r14 */
    movq %r15,  0x48(%rsp)  /* store %r15 to data->r15 */

    movq -0x8(%r10), %r9    /* load caller's return address */
    movq  %r9,  0x08(%rsp)  /* write return address to data->rip */

    movq %rcx,  0x58(%rsp)  /* store hidden address to data->tranferAddress */
    movq %r10,  0x60(%rsp)  /* store stack address of the callee to data->stack */

    movq %rdx,  %r9         /* move proc address to r9 */
    movq %r11,  %rcx        /* arg[0]: fiber    */
    movq %r8,   %rdx        /* arg[1]: param    */
    movq 0x08(%r11), %r8    /* arg[2]: userData */

    movq %r11,  %rbx        /* save fiber to rbx */

    call *%r9               /* call proc */

    /*
            NOTE: If proc does not return directly, and this context is instead jumped to,
                  it will bypass the rest of afiber_fork and return directly to the caller
                  one up the stack.
    */

    movq %rax,  %rcx        /* copy return value of proc to %rcx (retValue) */

    movq 0x58(%rsp), %rax   /* copy hidden address to return value */
    movq %rbx,  0x00(%rax)  /* transfer->parent = fiber */
    movq %rcx,  0x08(%rax)  /* transfer->param = retValue */

    movq 0x10(%rsp), %rbx   /* restore rbx; no need to restore any of the others, given that execution reached this point by returning from proc, and rbx is callee saved. */

    movq 0x08(%rsp), %rdx   /* load return address */
    movq 0x60(%rsp), %rsp   /* reset stack address */

    jmp *%rdx

.seh_endproc

.section .drectve
.ascii " -export:\"afiber_fork\""