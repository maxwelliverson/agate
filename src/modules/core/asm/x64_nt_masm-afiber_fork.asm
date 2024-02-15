
.code


$tibFiberField = 40

; generate function table entry in .pdata and unwind information in
afiber_fork PROC EXPORT FRAME
    ; .xdata for a function's structured exception handling unwind behavior
.endprolog


    ;   rcx: agt_fiber_transfer_t* hiddenAddress
    ;   rdx: agt_fiber_proc_t      proc
    ;   r8:  agt_fiber_param_t     param
    ;   r9:  agt_fiber_flags_t     flags



    lea r10, QWORD PTR [rsp+8]   ; load the caller's stack address to %r10.

    ; the following is the preferred way of accessing the TEB, but I don't understand whether or not this is still relevant in the modern x64 era...
    ;    mov r11, QWORD PTR gs:48
    ;    mov r11, QWORD PTR [r11+32]

    mov r11, QWORD PTR gs:[$tibFiberField]   ; load TEB->fiber to r11

    and rsp, -64                 ; align stack to 64 bytes

    test r9b, 1                  ; if (flags & AGT_FIBER_SAVE_EXTENDED_STATE) != 0
    je SHORT $LN2@afiber_fork    ;    goto $LN2@afiber_fork

    sub  rsp, QWORD PTR [r11+24] ; allocate fiber->saveRegionSize bytes to stack
    xor     rax, rax                   ; zero rax
    mov     QWORD PTR [rsp+512], rax   ; zero the value of XSTATE_BV
    mov     QWORD PTR [rsp+520], rax   ; zero the value of XCOMP_BV
    mov     QWORD PTR [rsp+528], rax   ; zero bytes 16:23 of the XSAVE header (for whatever reason, if this doesn't happen, a GP exception is raised by the processor when xrstor is executed)
    push rdx                     ; copy rdx to stack (as the register is needed as an implicit parameter to xsave)
    mov  rax, QWORD PTR [r11+16] ; load storeStateFlags to rax
    mov  rdx, rax                ; copy rax to rdx (flags)
    shr  rdx, 32                 ; shift rdx 32 bits to the right (now eax contains the low 32 flag bits, and edx contains the high 32 flag bits)
    xsaveopt64 QWORD PTR [rsp+8] ; save extended state to data (implicit params eax and edx, offset of 8 bytes because rdx was pushed to the stack)
    pop  rdx

$LN2@afiber_fork:

    sub rsp, 128                 ; allocate 128 bytes to stack

    mov rax, QWORD PTR [r11]     ; load fiber->privateData to rax
    mov QWORD PTR [r11], rsp     ; write data to fiber->privateData
    mov QWORD PTR [rsp], rax     ; write fiber->privateData to data->prevData

    mov DWORD PTR [rsp+80], r9d  ; store flags to data->saveFlags

    mov QWORD PTR [rsp+16], rbx  ; store rbx to data->rbx
    mov QWORD PTR [rsp+24], rbp  ; store rbp to data->rbp
    mov QWORD PTR [rsp+32], rdi  ; store rdi to data->rdi
    mov QWORD PTR [rsp+40], rsi  ; store rsi to data->rsi
    mov QWORD PTR [rsp+48], r12  ; store r12 to data->r12
    mov QWORD PTR [rsp+56], r13  ; store r13 to data->r13
    mov QWORD PTR [rsp+64], r14  ; store r14 to data->r14
    mov QWORD PTR [rsp+72], r15  ; store r15 to data->r15

    mov r9, QWORD PTR [r10-8]    ; load caller's return address
    mov QWORD PTR [rsp+8], r9    ; write return address to data->rip

    mov QWORD PTR [rsp+88], rcx  ; store hidden address to data->tranferAddress
    mov QWORD PTR [rsp+96], r10  ; store stack address of the callee to data->stack

    mov r9, rdx                  ; move proc address to r9
    mov rcx, r11                 ; arg[0] = fiber
    mov rdx, r8                  ; arg[1] = param
    mov r8, QWORD PTR [r11+8]    ; arg[2] = userData

    mov rbx, r11                 ; save fiber to rbx

    sub rsp, 32                ; allocate stack space for arguments (24 bytes for 3 qword args, plus an additional 8 to retain 16 byte stack alignment).

    call r9                      ; call proc

    add rsp, 32                ; deallocate stack space for arguments

    ; NOTE: If proc does not return directly, and this context is instead jumped to,
    ;       it will bypass the rest of afiber_fork and return directly to the caller
    ;       one up the stack.

    mov rcx, rax            ; copy return value of proc to rcx (retValue)

    mov rax, QWORD PTR [rsp+88] ; copy hidden address to return value
    mov QWORD PTR [rax], rbx    ; transfer->parent = fiber
    mov QWORD PTR [rax+8], rcx  ; transfer->param = retValue

    mov rbx, QWORD PTR [rsp+16] ; restore rbx (no need to restore any of the others, given that execution reached this point by returning from proc, and rbx is callee saved)

    mov rdx, QWORD PTR [rsp+8]  ; load return address
    mov rsp, QWORD PTR [rsp+96] ; reset stack address

    jmp rdx                     ; return

afiber_fork ENDP
END