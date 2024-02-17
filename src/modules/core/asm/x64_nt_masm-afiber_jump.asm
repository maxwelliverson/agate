
.code

$tibFiberField = 40

; generate function table entry in .pdata and unwind information in
afiber_jump PROC EXPORT FRAME
    ; .xdata for a function's structured exception handling unwind behavior
.endprolog


    ;   rcx: agt_fiber_t       targetFiber
    ;   rdx: agt_fiber_param_t param

    ;   note that non-volatile registers are used as scratch registers without saving their contents,
    ;   this is okay because this context is abandonned once the jump is complete (ie. this function does not return)

    mov     r8, QWORD PTR gs:[48]      ; load the local NT_TIB to r8

    mov     r10, QWORD PTR [r8+$tibFiberField]     ; load fromFiber to r10
    mov     QWORD PTR[r8+$tibFiberField],  rcx     ; set fiber storage field of NT_TIB to toFiber

    mov     rax, QWORD PTR [rcx+48]
    mov     QWORD PTR[r8+5240], rax    ; restore deallocation stack

    mov     rax, QWORD PTR [rcx+32]
    mov     QWORD PTR[r8+8], rax       ; restore stack base

    mov     rax, QWORD PTR [rcx+40]
    mov     QWORD PTR[r8+16], rax      ; restore stack limit

    mov     r9, QWORD PTR [rcx]        ; set r9 to toFiber->data

    mov     rax, QWORD PTR [r9]        ;
    mov     QWORD PTR [rcx], rax       ; toFiber->data = data->prevData
    mov     rax, QWORD PTR [r9+8]      ; load userFlags:controlFlags to rax
    mov     rsp, QWORD PTR [r9+16]     ; restore rsp
    mov     r11, QWORD PTR [r9+24]     ; load the instruction pointer to r11 for now

    test    al, 1                     ; if (controlFlags & FIBER_CONTROL_LOOP_CTX) == 0 (implicit operand of al register)
    je      SHORT $LN2@afiber_jump     ;   jump to $LN2@afiber_jump
           ; Here we know that this is jumping to a loop, so we don't need to restore any further state
           ; We also need to fill the parameters for calling the loop proc
           ;   rcx: fromFiber (move from r10)
           ;   rdx: param     (already done)
           ;   r8:  userData  (load from toFiber)
    mov     r8, QWORD PTR [rcx+8]      ; load toFiber->userData to r8
    mov     rcx, r10                   ; load fromFiber to rcx
    jmp     r11                        ; call loop proc


$LN2@afiber_jump:
    shr     rax,    32                 ; move userFlags into eax
    test    al, 1                     ; if (userFlags & AGT_FIBER_SAVE_EXTENDED_STATE) == 0
    je      SHORT $LN3@afiber_jump     ;   jump to $LN3@afiber_jump
    mov     rbx,    rdx                ; store the current value of rdx (param) in rbx for now (xrstor uses rdx as an operand)
    mov     rax, QWORD PTR [rcx+16]    ; load storeStateFlags to rax
    mov     rdx,    rax                ; copy storeStateFlags to rdx
    shr     rdx,    32                 ; shift the high 32 bits of the mask down to fit into edx
    xrstor64 QWORD PTR [r9+128]        ; restore extended state from toFiber->data (implicit params eax and edx)
    mov     rdx,    rbx                ; restore toFiber to rdx
$LN3@afiber_jump:

    ; rax: N/A,      rcx: toFiber,       rdx: param,
    ; r8: NT_TIB,    r9:  toFiber->data, r10: fromFiber,  r11: N/A

                               ; let data = toFiber->data
                               ;
    mov     rax, QWORD PTR [r9+32]     ; load transfer address
    mov     rbx, QWORD PTR [r9+40]     ; restore rbx
    mov     rbp, QWORD PTR [r9+48]     ; restore rbp
    mov     rdi, QWORD PTR [r9+56]     ; restore rdi
    mov     rsi, QWORD PTR [r9+64]     ; restore rsi
    mov     r12, QWORD PTR [r9+72]     ; restore r12
    mov     r13, QWORD PTR [r9+80]     ; restore r13
    mov     r14, QWORD PTR [r9+88]     ; restore r14
    mov     r15, QWORD PTR [r9+96]     ; restore r15
    mov     QWORD PTR [rax],   r10     ; copy fromFiber to transfer->source
    mov     QWORD PTR [rax+8], rdx     ; copy param to transfer->param
    jmp     r11                        ; return

afiber_jump ENDP
END