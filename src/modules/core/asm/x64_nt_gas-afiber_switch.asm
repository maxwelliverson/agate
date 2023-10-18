.file	"x64_nt_gas-afiber_switch.asm"
.text
.p2align 4,,15
.globl	afiber_switch
.def	afiber_switch
	.scl	2
    .type	32
.endef
.seh_proc afiber_switch
afiber_switch:
.seh_endprologue

    /*
       %rcx: agt_fiber_transfer_t*  hiddenAddress
       %rdx: agt_fiber_t            toFiber
       %r8:  agt_fiber_param_t      param
       %r9:  agt_fiber_save_flags_t flags
    */

    pushq   %rbp               /* push rbp */
    movq    %rsp, %rbp         /* copy stack ptr to rbp */
    andq    $-64, %rsp         /* align stack to 64 bytes */

    movq    %gs:(0x30), %r11   /* load NT_TIB */
    movq    0x20(%r11), %r10   /* load thisFiber to r11 from the fiber local storage field of NT_TIB */

    testb $1, %r9l             /* if (flags & AGT_FIBER_SAVE_EXTENDED_STATE) == 0 */
    jz .L_post_xsave           /*   jump to .L_post_xsave */
    subq    0x20(%rcx), %rsp   /* subtract fromFiber->saveRegionSize from stack pointer */
    movq    %rdx, %r11         /* temporarily store toFiber in r11 (we need to use rdx for xsaveopt) */
    movq    0x18(%rcx), %rax   /* load storeStateFlags to rax */
    movq    %rax, %rdx         /* copy storeStateFlags to rdx */
    shrq    $32, %rdx          /* shift the high 32 bits of the mask down to fit into edx */
    xsaveopt64 (%rsp)          /* save extended state to data (implicit params %eax and %edx) */
    movq    %r11, %rdx         /* restore toFiber to rdx */
.L_post_xsave:
    subq  $0x80,   %rsp        /* allocate 128 bytes to stack for fiber_data */
    movq  0x10(%r10), %r11
    movq  %r11, 0x00(%rsp)     /* set data->prevData to fromFiber->data */
    movq  %rsp, 0x10(%r10)     /* set fromFiber->data to data */
    leaq  .L_rstor(%rip), %r11
    movq  %rbx, 0x10(%rsp)     /* store %rbx to data->rbx */
    movq  %rbp, 0x18(%rsp)     /* store %rbp to data->rbp */
    movq  %rdi, 0x20(%rsp)     /* store %rdi to data->rdi */
    movq  %r11, 0x08(%rsp)     /* store address of .switch_rstor label to data->rip */
    movq  %rsi, 0x28(%rsp)     /* store %rsi to data->rsi */
    movq  %r12, 0x30(%rsp)     /* store %r12 to data->r12 */
    movq  %r13, 0x38(%rsp)     /* store %r13 to data->r13 */
    movq  %r14, 0x40(%rsp)     /* store %r14 to data->r14 */
    movq  %r15, 0x48(%rsp)     /* store %r15 to data->r15 */
    mov   %r9d, 0x50(%rsp)     /* store flags to data->saveFlags */
    movq  %rcx, 0x58(%rsp)     /* store hidden address to data->tranferAddress */

    movq  %rdx,       %rcx     /* copy toFiber to the first parameter */
    movq  (%rdx),     %rdx     /* load toFiber->data */
    movq  0x08(%rdx), %r11     /* load toFiber->data->rip */
    movq  $1,         %r9      /* indicate that the jump is a regular jump */
    jmp  *%r11                 /* jump to toFiber's context! */

    /* jump protocol: [rcx: thisFiber, rdx: data, r8: param, r9: jumpKind, r10: parentFiber] */

.L_rstor:

    movq  %gs:(0x30), %r11   /* load NT_TIB */

    movq  %rcx, 0x20(%r11)  /* store agt_fiber_t to the fiber local storage field in NT_TIB. This is unsafe if it is otherwise possible for the current thread to ever call the win32 function ConvertThreadToFiber */

    movq 0x38(%rcx), %rax
    movq %rax, 0x1478(%r11) /* restore deallocation stack */

    movq 0x28(%rcx), %rax
    movq %rax, 0x08(%r11)   /* restore stack base */

    movq 0x30(%rcx), %rax
    movq %rax, 0x10(%r11)   /* restore stack limit */

    movq 0x10(%rdx), %rbx  /* restore rbx */
    movq 0x20(%rdx), %rdi  /* restore rdi */
    movq 0x28(%rdx), %rsi  /* restore rsi */
    movq 0x30(%rdx), %r12  /* restore r12 */
    movq 0x38(%rdx), %r13  /* restore r13 */
    movq 0x40(%rdx), %r14  /* restore r14 */
    movq 0x48(%rdx), %r15  /* restore r15 */
    mov  0x50(%rdx), %r11d /* load flags to r11 */

    testb $1, %r10l
    jz .L_post_xrstor
    movq    %rdx, %r11         /* temporarily store data in r11 (we need to use rdx for xrstor) */
    movq    0x18(%rcx), %rax   /* load storeStateFlags to rax */
    movq    %rax, %rdx         /* copy storeStateFlags to rdx */
    shrq    $32, %rdx          /* shift the high 32 bits of the mask down to fit into edx */
    xrstor64 0x80(%r11)        /* restore extended state */
    movq    %r11, %rdx         /* restore data to rdx */
.L_post_xrstor:

    movq 0x58(%rdx), %rax      /* restore hidden address and set as return value */


    movq 0x18(%rdx), %rsp      /* finally, restore stack pointer obtained from stored frame pointer */
    movq (%rdx), %r11
    movq %r11,  (%rcx)         /* pop data from context stack */

    movq %r10, 0x00(%rax)      /* set transfer->parent to fromFiber */
    movq %r8,  0x08(%rax)      /* set transfer->param to param */


    popq %rbp                  /* restore rbp */
    retq

.seh_endproc

.section .drectve
.ascii " -export:\"afiber_switch\""