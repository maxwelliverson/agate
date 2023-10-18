.file  "x64_nt_gas-afiber_init.asm"
.text
.p2align 4,,15
.globl	afiber_init
.def	afiber_init
	.scl	2
    .type	32
.endef
.seh_proc afiber_init
afiber_init:
.seh_endprologue

    movq   0x28(%rcx), %r10    /* load fiber's stack base to %r10 */
    subq   $0x80,      %r10    /* allocate 128 bytes to the base of the fiber stack. */

    xorq   %rax,       %rax    /* zero the return value */
    movq   %rax,  0x00(%r10)   /* set data->prevData to null to indicate this as being the top of the context stack */
    mov    %eax,  0x50(%r10)   /* set flags to 0 */


    /* jump protocol: [rcx: thisFiber, rdx: data, r8: param, r9: jumpKind, r10: parentFiber] */

    /* jump: [rax: transfer*, rcx: thisFiber, rdx: param, r8: userData, r9: data, r10: jumpKind] */

.L_trampoline:



    pushq   %rbp               /* push rbp */
    movq    %rsp, %rbp         /* copy stack ptr to rbp */
    andq    $-64, %rsp         /* align stack to 64 bytes */

    testb $1, %r9l             /* if (flags & AGT_FIBER_SAVE_EXTENDED_STATE) == 0 */
    jz .L_post_xsave           /*   jump to .L_post_xsave */
    subq    0x20(%rcx), %rsp   /* subtract fromFiber->saveRegionSize from stack pointer */
    movq    %rdx, %r10         /* temporarily store toFiber in r10 (we need to use rdx for xsaveopt) */
    movq    0x18(%rcx), %rax   /* load storeStateFlags to rax */
    movq    %rax, %rdx         /* copy storeStateFlags to rdx */
    shrq    $32, %rdx          /* shift the high 32 bits of the mask down to fit into edx */
    xsaveopt64 (%rsp)          /* save extended state to data (implicit params %eax and %edx) */
    movq    %r10, %rdx         /* restore toFiber to rdx */



.seh_endproc

.section .drectve
.ascii " -export:\"afiber_init\""

