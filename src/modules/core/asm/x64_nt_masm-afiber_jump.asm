
.code

$tibFiberField = 40

; generate function table entry in .pdata and unwind information in
afiber_jump PROC EXPORT FRAME
    ; .xdata for a function's structured exception handling unwind behavior
.endprolog


    ;   rcx: agt_fiber_t       targetFiber
    ;   rdx: agt_fiber_param_t param

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
    mov     r11d, DWORD PTR [r9+80]    ; load toFiber->data->flags to r11
    test    r11b,   1                  ; if (flags & AGT_FIBER_SAVE_EXTENDED_STATE) == 0
    je SHORT $LN2@afiber_jump          ;   jump to $LN2@afiber_jump
    mov     r11,    rdx                ; store the current value of rdx (param) in r11 for now (xrstor uses rdx as an operand)
    mov     rax, QWORD PTR [rcx+16]    ; load storeStateFlags to rax
    mov     rdx,    rax                ; copy storeStateFlags to rdx
    shr     rdx,    32                 ; shift the high 32 bits of the mask down to fit into edx
    xrstor64 QWORD PTR [r9+128]        ; restore extended state from toFiber->data (implicit params eax and edx)
    mov     rdx,    r11                ; restore toFiber to rdx
$LN2@afiber_jump:

    ; rax: N/A,      rcx: toFiber,       rdx: param,
    ; r8: NT_TIB,    r9:  toFiber->data, r10: fromFiber,  r11: N/A

                               ; let data = toFiber->data
                               ;
    mov r11, QWORD PTR [r9]    ;
    mov QWORD PTR [rcx], r11   ; toFiber->data = data->prevData
    mov rbx, QWORD PTR [r9+16] ; restore rbx
    mov rbp, QWORD PTR [r9+24] ; restore rbp
    mov rdi, QWORD PTR [r9+32] ; restore rdi
    mov rsi, QWORD PTR [r9+40] ; restore rsi
    mov r12, QWORD PTR [r9+48] ; restore r12
    mov r13, QWORD PTR [r9+56] ; restore r13
    mov r14, QWORD PTR [r9+64] ; restore r14
    mov r15, QWORD PTR [r9+72] ; restore r15
    mov rax, QWORD PTR [r9+88] ; load transfer address
    mov rsp, QWORD PTR [r9+96] ; load stack address

    ; rax: transfer*, rcx: toFiber,       rdx: param,
    ; r8: NT_TIB,     r9:  toFiber->data, r10: fromFiber,  r11: N/A

    mov   r8,                rcx        ; copy toFiber to r8
    mov   rcx,               r10        ; copy fromFiber to rcx
    test  rax,               rax        ; if transfer != nullptr
    je SHORT $LN3@afiber_jump           ;
    mov   QWORD PTR [rax],   rcx        ;   copy fromFiber to transfer->source
    mov   QWORD PTR [rax+8], rdx        ;   copy param to transfer->param
$LN3@afiber_jump:
    mov   r8,   QWORD PTR [r8+8]        ; copy toFiber->userData to r8

    jmp QWORD PTR [r9+8]                ; jump to address




afiber_jump ENDP
END