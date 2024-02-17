EXTERN afiber_jump : PROC
.code


$tibFiberField = 40

; struct fiber_stack_info
$address      = 0
$reserveSize  = 8
$commitSize   = 16

; struct agt_fiber_st
$privateData     = 0
$userData        = 8
$storeStateFlags = 16
$saveRegionSize  = 24
$stackBase       = 32
$stackLimit      = 40
$deallocStack    = 48

 ; struct fiber_data
$prevData     = 0
$controlFlags = 8
$userFlags    = 12
$stack        = 16
$rip          = 24


; generate function table entry in .pdata and unwind information in
afiber_init PROC EXPORT FRAME
    ; .xdata for a function's structured exception handling unwind behavior
    .endprolog

    ; struct fiber_stack_info {
    ;   void*  address;
    ;   size_t reserveSize;
    ;   size_t commitSize;
    ; }

    ; rcx: agt_fiber_t fiber
    ; rdx: agt_fiber_proc_t proc
    ; r8:  const fiber_stack_info* stackInfo

    mov    r9,  QWORD PTR [r8]                ; load stackInfo->address to r9
    mov    QWORD PTR [rcx+$deallocStack], r9  ; fiber->deallocationStack = stackInfo->address
    add    r9, QWORD PTR [r8+$reserveSize]    ; add stackInfo->reserveSize to address == stackBase
    mov    r10, r9                            ; copy stackBase to r11
    mov    QWORD PTR [rcx+$stackBase], r9     ; fiber->stackBase = stackBase
    sub    r10, QWORD PTR [r8+$commitSize]    ;
    mov    QWORD PTR [rcx+$stackLimit], r10   ; fiber->stackLimit = stackBase - stackInfo->commitSize

    mov    r11d, DWORD PTR [r8+28]    ; load stackInfo->randomOffset
    sub    r9,   r11                  ; subtract random offset from stackBase

    and    r9,   -16                  ; align stack to 16 bytes
    sub    r9,   32                   ; allocate 32 bytes for the loop context
                                      ; for added security, it might be best to add a random offset to the stackBase here...
           ; r9 holds stackBase
    ; and    r9,                -64     ; align stack to 64 bytes (should already be, but just to be safe!)
    ; sub    r9,                 128    ; allocate 128 bytes for the simple context
    mov    eax, 1                       ; set rax to 1
    lea    r10, QWORD PTR [r9-40]     ; load next stack frame address to r10
    mov    QWORD PTR [rcx],    r9     ; set fiber->privateData to this data.
    mov    QWORD PTR [r9],     r9     ; set data->prevData to data (similarly to afiber_loop, this causes the context to not be popped when it is jumped to).
    ; inc    eax
    mov    QWORD PTR [r9+8],    rax    ; set data->controlFlags to 0x1 (FIBER_CONTROL_LOOP_CTX) and data->userFlags to 0
    ; mov    DWORD PTR [r9+12],   eax    ; set data->userFlags to 0
    ; inc    eax                        ; set eax to 0x1
    ; mov    DWORD PTR [r9+8],  eax    ; set data->controlFlags to 0x1 (FIBER_CONTROL_LOOP_CTX)
    mov    QWORD PTR [r9+16], r10     ; store next stack frame address to data->stack
    mov    QWORD PTR [r9+24],  rdx    ; set data->rip to proc

    mov    r11, $LN2@afiber_init      ; load address of $LN2$afiber_init label
    mov    QWORD PTR [r10+32], rcx     ; write thisFiber to the stack (for fast access in $LN2@afiber_init)
    mov    QWORD PTR [r10],   r11     ; manually write a 'return address' for when proc is 'called'
    ret                               ; return!

    ; if the fiber proc ever returns, it'll return as though from the following instruction:
    ; call rdx
$LN2@afiber_init:

    mov r9, QWORD PTR[rsp+24]         ; retrieve the fiber pointer stored above (mov QWORD PTR [r10+32], rcx)
    ; add rsp, 24                       ; deallocate
    ; pop r9                            ; retrieve the fiber pointer stored above
    mov rdx, rax                      ; move the return value (parameter) to rdx
    mov rcx, QWORD PTR [r9+56]        ; load fiber->defaultJumpTarget to rcx
    jmp afiber_jump                   ; call afiber_jump(fiber->defaultJumpTarget, param)
                                      ; TODO: Try doing the jump inline and benchmark performance?

afiber_init ENDP
END