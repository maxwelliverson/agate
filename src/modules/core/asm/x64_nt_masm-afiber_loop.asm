
.code

$tibFiberField = 40

; generate function table entry in .pdata and unwind information in
afiber_loop PROC EXPORT FRAME
    ; .xdata for a function's structured exception handling unwind behavior
.endprolog

    ;   rcx: agt_fiber_proc_t   proc
    ;   rdx: agt_fiber_param_t  param
    ;   r8:  agt_fiber_flags_t  flags


    push rbp
    mov  rbp,         rsp        ; store the stack frame base address

    ; the following is the preferred way of accessing the TEB, but I don't understand whether or not this is still relevant in the modern x64 era...
    ;    mov r11, QWORD PTR gs:48
    ;    mov r11, QWORD PTR [r11+32]

    mov r9, QWORD PTR gs:[$tibFiberField]    ; load NT_TIB->fiber to r11 (thisFiber)

    and rsp, -64                 ; align stack to 64 bytes

    test r8b, 1                  ; if (flags & AGT_FIBER_SAVE_EXTENDED_STATE) != 0
    je SHORT $LN2@afiber_loop    ;    goto $LN2@afiber_loop

    ; ALLOCATE AND ZERO MEMORY ON STACK
    sub  rsp, QWORD PTR [r9+24]  ; allocate fiber->saveRegionSize bytes to stack
    xor  rax, rax
    mov QWORD PTR [rsp+512], rax   ; zero bytes 0:7 of xsave header
    mov QWORD PTR [rsp+520], rax   ; zero bytes 8:15 of xsave header
    mov QWORD PTR [rsp+528], rax   ; zero bytes 16:23 of xsave header
    ; mov  r10, rcx
    ; mov  r11, rdi
    ; mov  rcx, QWORD PTR [r9+24]  ; load fiber->saveRegionSize to rcx
    ; sub  rsp, rcx
    ; mov  rdi, rsp                ; copy the new stack pointer to rdi
    ; shr  rcx, 3                  ; divide by 8 to get number of quadwords
    ; rep stosq                    ; zero the save region!!!
    ; mov  rcx, r10                ; restore rcx
    ; mov  rdi, r11                ; restore rdi

    mov  r10, rdx                ; copy rdx to r10 (as the register is needed as an implicit parameter to xsave)
    mov  rax, QWORD PTR [r9+16]  ; load storeStateFlags to rax
    ; sub  rsp, QWORD PTR [r9+24]  ; allocate fiber->saveRegionSize bytes to stack
    mov  rdx, rax                ; copy rax to rdx (flags)
    shr  rdx, 32                 ; shift rdx 32 bits to the right (now eax contains the low 32 flag bits, and edx contains the high 32 flag bits)
    xsaveopt64 QWORD PTR [rsp]   ; save extended state to data (implicit params eax and edx)
    mov  rdx, r10                ; restore rdx

$LN2@afiber_loop:

    sub rsp, 128                 ; allocate 128 bytes to stack

    shl r8,  32                  ; shift userFlags to the high 32 bits of r8
    inc r8b                      ; set controlFlags to 0x1 (FIBER_CONTROL_LOOP_CTX)
    lea r10, QWORD PTR [rsp-56]  ; load to r10 the address of what *will* be the next stack frame. (8 bytes for rax, 8 bytes for fiber, 8 bytes for the return address, 24 bytes for parameters, additional 8 bytes for alignment)
    mov rax, QWORD PTR [r9]      ; load fiber->privateData to rax
    mov QWORD PTR [r9], rsp      ; write data to fiber->privateData
    mov QWORD PTR [rsp], rsp     ; write data to data->prevData (this is so that jumping to this context does not pop the context from the stack. We will save the previous pointer externally so that only we can replace it).
    mov QWORD PTR [rsp+8], r8    ; write flags to data->controlFlags and data->userFlags
    mov QWORD PTR [rsp+16], r10  ; store next frame address to data->stack
    mov QWORD PTR [rsp+24], rcx  ; write proc to data->rip
    ; xor r11, r11                 ; zero r11
    ; mov QWORD PTR [rsp+32], r11  ; store null to data->tranferAddress (not really necessary tbh)
    mov QWORD PTR [rsp+40], rbx  ; store rbx to data->rbx
    mov QWORD PTR [rsp+48], rbp  ; store rbp to data->rbp
    mov QWORD PTR [rsp+56], rdi  ; store rdi to data->rdi
    mov QWORD PTR [rsp+64], rsi  ; store rsi to data->rsi
    mov QWORD PTR [rsp+72], r12  ; store r12 to data->r12
    mov QWORD PTR [rsp+80], r13  ; store r13 to data->r13
    mov QWORD PTR [rsp+88], r14  ; store r14 to data->r14
    mov QWORD PTR [rsp+96], r15  ; store r15 to data->r15

    push rax                     ; push old privateData to stack
    push r9                      ; push thisFiber to stack

    mov  r11, rcx                ; move proc address to r11
    mov  rcx, r9                 ; arg[0] = fiber
                                 ; arg[1] = param
    mov  r8, QWORD PTR [r9+8]    ; arg[2] = userData

    sub  rsp, 32                 ; allocate 32 bytes for the arguments (24 bytes for the 3 qword arguments, no need for any more as this maintains alignment)

    call r11                     ; call proc

    mov  rcx, QWORD PTR [rsp+32] ; restore thisFiber to rcx
    mov  r8,  QWORD PTR [rsp+40] ; restore prevData to r8

    ; add  rsp, 32                 ; deallocate 32 bytes on stack

    ; pop  rcx                     ; restore thisFiber to rcx
    ; pop  r8                      ; restore prevData to rdx



    lea     r9, QWORD PTR[rsp+48]      ; load address of data to r9

    mov     r10d, DWORD PTR [r9+12]    ; load userFlags to r10d
    test    r10b, 1                    ; if (flags & AGT_FIBER_SAVE_EXTENDED_STATE) != 0
    je SHORT $LN3@afiber_loop          ;   jump to $LN3@afiber_loop
    mov     rbx,    rax                ; store the current value of rax (retValue) in rbx for now (xrstor uses rdx as an operand)
    mov     rax, QWORD PTR [rcx+16]    ; load storeStateFlags to rax
    mov     rdx,    rax                ; copy storeStateFlags to rdx
    shr     rdx,    32                 ; shift the high 32 bits of the mask down to fit into edx
    xrstor64 QWORD PTR [r9+128]        ; restore extended state from toFiber->data (implicit params eax and edx)
    mov     rax,    rbx                ; restore toFiber to rdx
$LN3@afiber_loop:

    mov     QWORD PTR [rcx], r8        ; set fiber->privateData to previous data
                                       ; ignore transfer address
    mov     rbx, QWORD PTR [r9+40]     ; restore rbx
    mov     rsp, QWORD PTR [r9+48]     ; restore rbp to rsp (note this is the address to which the original value of rbp was pushed)
    mov     rdi, QWORD PTR [r9+56]     ; restore rdi
    mov     rsi, QWORD PTR [r9+64]     ; restore rsi
    mov     r12, QWORD PTR [r9+72]     ; restore r12
    mov     r13, QWORD PTR [r9+80]     ; restore r13
    mov     r14, QWORD PTR [r9+88]     ; restore r14
    mov     r15, QWORD PTR [r9+96]     ; restore r15

    pop rbp                            ; restore the prior value of rbp

    ret

    ; NOTE: When this context is jumped to, control goes directly to proc, as though proc had just been called,
    ;       but with the fromFiber and param arguments coming from the jump call.
    ;       It is only when proc returns normally that this context is popped and afiber_loop returns

    ; TODO: Add an internal flag to agt_fiber_flag_bits_t that indicates whether or not the context is a loop context.
    ;       There are enough differences between jumps to loop contexts and others that it'd probably make afiber_loops
    ;       much more efficient, as the only registers that would need to be restored on a *jump* are rsp and rip.
    ;       The tradeoff is that exiting the loop requires a full context restore.
    ;       Worth benchmarking to see if it does make a significant difference or not, but I expect it will.

    ; NOTE: The callee saved registers should be unchanged from before, so rbp should still hold the frame pointer

    ; mov rcx, QWORD PTR gs:[$tibFiberField]  ; load NT_TIB->fiber to rcx (thisFiber)
    ; mov rdx, QWORD PTR [rsp]    ; curious if a mov instruction is faster than pop, given we don't actually need to adjust the stack pointer here? (it will be overwritten by the frame pointer momentarily)
    ; pop rdx                     ; get previous value of fiber->privateData

    ; mov QWORD PTR [rcx], rdx    ; set fiber->privateData to previous data

    ; mov rsp, rbp                ; set stack pointer to frame base
    ; pop rbp                     ; restore the prior value of rbp

    ; ret                         ; return :)

afiber_loop ENDP
END