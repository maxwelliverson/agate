EXTERN  _exit:PROC
.code

; generate function table entry in .pdata and unwind information in
afiber_switch PROC BOOST_CONTEXT_EXPORT FRAME
    ; .xdata for a function's structured exception handling unwind behavior
    .endprolog


afiber_switch ENDP
END