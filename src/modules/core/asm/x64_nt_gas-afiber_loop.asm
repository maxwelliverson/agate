.file	"x64_nt_gas-afiber_loop.asm"
.text
.p2align 4,,15
.globl	afiber_loop
.def	afiber_loop
	.scl	2
    .type	32
.endef
.seh_proc afiber_loop
afiber_loop:
.seh_endprologue

    # %rcx: agt_fiber_t            fiber
    # %rdx: agt_fiber_proc_t       proc
    # %r8:  agt_fiber_param_t      param
    # %r9:  agt_fiber_save_flags_t flags

