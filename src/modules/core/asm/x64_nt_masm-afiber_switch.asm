.code

$tibFiberField = 40

; generate function table entry in .pdata and unwind information in
afiber_switch PROC EXPORT FRAME
    ; .xdata for a function's structured exception handling unwind behavior
    .endprolog


       ; rcx: agt_fiber_transfer_t*  hiddenAddress
       ; rdx: agt_fiber_t            toFiber
       ; r8:  agt_fiber_param_t      param
       ; r9:  agt_fiber_save_flags_t flags

    push    rbp                        ; push rbp
    mov     rbp,   rsp                 ; copy stack ptr to rbp
    and     rsp,   -64                 ; align stack to 64 bytes

    mov     r10,   QWORD PTR gs:[$tibFiberField]   ; r10 = GetCurrentFiber(),   load thisFiber from the fiber local storage field of NT_TIB

    test    r9b,   1                   ; if (flags & AGT_FIBER_SAVE_EXTENDED_STATE) == 0
    je SHORT $LN2@afiber_switch        ;   jump to $LN2@afiber_switch
                                       ; SAVE EXTENDED REGISTERS (x87, SSE2, SSE4.1/2, AVX, etc.)
    sub     rsp, QWORD PTR [r10+24]    ; subtract fromFiber->saveRegionSize from stack pointer
    xor     rax, rax                   ; zero rax
    mov     QWORD PTR [rsp+512], rax   ; zero the value of XSTATE_BV
    mov     QWORD PTR [rsp+520], rax   ; zero the value of XCOMP_BV
    mov     QWORD PTR [rsp+528], rax   ; zero bytes 16:23 of the XSAVE header (for whatever reason, if this doesn't happen, a GP exception is raised by the processor when xrstor is executed)
    mov     r11,    rdx                ; store the current value of rdx (toFiber) in r11 for now (xsaveopt uses rdx as an operand)
    mov     rax, QWORD PTR [r10+16]    ; load storeStateFlags to rax
    mov     rdx,    rax                ; copy storeStateFlags to rdx
    shr     rdx,    32                 ; shift the high 32 bits of the mask down to fit into edx
    xsaveopt64 QWORD PTR [rsp]         ; save extended state to data (implicit params eax and edx)
    mov     rdx,    r11                ; restore toFiber to rdx

$LN2@afiber_switch:
                                       ; SAVE GENERAL PURPOSE REGISTERS
                                       ;
    sub     rsp,   128                 ; std::byte data[128],       allocate 128 bytes for register storage
    mov     r11,   QWORD PTR [r10]  ;
    mov     QWORD PTR [rsp],    r11    ; data->prevData = fromFiber->data
    mov     r11,   QWORD PTR [rbp+8]   ;
    mov     QWORD PTR [rsp+8],  r11    ; data->rip = returnAddress
    mov     QWORD PTR [r10], rsp       ; fromFiber->data = data
    mov     QWORD PTR [rsp+16], rbx    ; store rbx to data->rbx
    mov     r11,   QWORD PTR [rbp]     ;
    mov     QWORD PTR [rsp+24], r11    ; store rbp to data->rbp
    mov     QWORD PTR [rsp+32], rdi    ; store rdi to data->rdi
    mov     QWORD PTR [rsp+40], rsi    ; store rsi to data->rsi
    mov     QWORD PTR [rsp+48], r12    ; store r12 to data->r12
    mov     QWORD PTR [rsp+56], r13    ; store r13 to data->r13
    mov     QWORD PTR [rsp+64], r14    ; store r14 to data->r14
    mov     QWORD PTR [rsp+72], r15    ; store r15 to data->r15
    mov     DWORD PTR [rsp+80], r9d    ; store flags to data->saveFlags
    mov     QWORD PTR [rsp+88], rcx    ; store hidden address to data->tranferAddress
    lea     rbp, QWORD PTR [rbp+16]    ; load effective address of stack upon return!
    mov     QWORD PTR [rsp+96], rbp    ; store stack address to data->stack

    ; swwwiiiitccchhhhh

    ; rax: N/A,      rbx: N/A,  rcx: fromHiddenAddress, rdx: toFiber,
    ; r8: param,     r9: flags, r10: fromFiber,         r11: N/A,
    ; r12: N/A,      r13: N/A,  r14: N/A,               r15: N/A,
    ; rdi: N/A,      rsi: N/A,  rbp: oldStack - 16,     rsp: fromData


    mov     r9, QWORD PTR gs:[48]      ; load the local NT_TIB to r9

    mov     QWORD PTR[r9+$tibFiberField],  rdx     ; set fiber storage field of NT_TIB to toFiber

    mov     rax, QWORD PTR [rdx+48]
    mov     QWORD PTR[r9+5240], rax    ; restore deallocation stack

    mov     rax, QWORD PTR [rdx+32]
    mov     QWORD PTR[r9+8], rax       ; restore stack base

    mov     rax, QWORD PTR [rdx+40]
    mov     QWORD PTR[r9+16], rax      ; restore stack limit

    ; rax: N/A,      rbx: N/A,   rcx: fromHiddenAddress, rdx: toFiber,
    ; r8: param,     r9: NT_TIB, r10: fromFiber,         r11: N/A,
    ; r12: N/A,      r13: N/A,   r14: N/A,               r15: N/A,
    ; rdi: N/A,      rsi: N/A,   rbp: oldStack - 16,     rsp: fromData

    mov     r9, QWORD PTR [rdx]        ; set r9 to toFiber->data
    mov     r11d, DWORD PTR [r9+80]    ; load toFiber->data->flags to r11
    test    r11b,   1                  ; if (flags & AGT_FIBER_SAVE_EXTENDED_STATE) == 0
    je SHORT $LN3@afiber_switch        ;   jump to $LN3@afiber_switch
    mov     r11,    rdx                ; store the current value of rdx (toFiber) in r11 for now (xrstor uses rdx as an operand)
    mov     rax, QWORD PTR [rdx+16]    ; load storeStateFlags to rax
    mov     rdx,    rax                ; copy storeStateFlags to rdx
    shr     rdx,    32                 ; shift the high 32 bits of the mask down to fit into edx
    xrstor64 QWORD PTR [r9+128]        ; restore extended state from toFiber->data (implicit params eax and edx)
    mov     rdx,    r11                ; restore toFiber to rdx

$LN3@afiber_switch:

    ; rax: N/A,      rbx: N/A,          rcx: fromHiddenAddress, rdx: toFiber,
    ; r8: param,     r9: toFiber->data, r10: fromFiber,         r11: N/A

                               ; let data = toFiber->data
                               ;
    mov r11, QWORD PTR [r9]    ;
    mov QWORD PTR [rdx], r11   ; toFiber->data = data->prevData
    mov rbx, QWORD PTR [r9+16] ; restore rbx
    mov rbp, QWORD PTR [r9+24] ; restore rbp
    mov rdi, QWORD PTR [r9+32] ; restore rbx
    mov rsi, QWORD PTR [r9+40] ; restore rbp
    mov r12, QWORD PTR [r9+48] ; restore rbx
    mov r13, QWORD PTR [r9+56] ; restore rbp
    mov r14, QWORD PTR [r9+64] ; restore rbx
    mov r15, QWORD PTR [r9+72] ; restore rbp
    mov rax, QWORD PTR [r9+88] ; load transfer address
    mov rsp, QWORD PTR [r9+96] ; load stack address

    ; rax: transferAddress, rbx: N/A,          rcx: fromHiddenAddress, rdx: toFiber,
    ; r8: param,            r9: toFiber->data, r10: fromFiber,         r11: N/A

    mov   rcx,               r10        ; copy fromFiber to rcx
    mov   r11,               rdx        ; copy toFiber to r11
    mov   rdx,               r8         ; copy param to rdx
    test  rax,               rax        ; if transfer != nullptr
    je SHORT $LN4@afiber_switch         ;
    mov   QWORD PTR [rax],   rcx        ;   copy fromFiber to transfer->source
    mov   QWORD PTR [rax+8], rdx        ;   copy param to transfer->param
$LN4@afiber_switch:
    mov   r8,   QWORD PTR [r11+8]       ; copy toFiber->userData to r8

    jmp QWORD PTR [r9+8]                ; jump to address


    ; rax: toFiber->data->transferAddress, rcx: fromFiber, rdx: param, r8: toFiber->userData, rsp: toFiber->data->stack

afiber_switch ENDP
END