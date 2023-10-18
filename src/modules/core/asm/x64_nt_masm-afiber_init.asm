EXTERN  _exit : PROC
.code

; generate function table entry in .pdata and unwind information in
make_fcontext PROC BOOST_CONTEXT_EXPORT FRAME
    ; .xdata for a function's structured exception handling unwind behavior
    .endprolog


make_fcontext ENDP
END