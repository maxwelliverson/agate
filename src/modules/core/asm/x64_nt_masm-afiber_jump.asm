
.code

; generate function table entry in .pdata and unwind information in
afiber_fork PROC EXPORT FRAME
    ; .xdata for a function's structured exception handling unwind behavior
.endprolog


    ;   rcx: agt_fiber_t       targetFiber
    ;   rdx: agt_fiber_param_t param

    mov r8, QWORD PTR gs:48      ; load NT_TEB




afiber_fork ENDP
END