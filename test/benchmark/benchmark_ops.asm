;EXTERN ___std_smf_riemann_zeta@8
.code


; generate function table entry in .pdata and unwind information in
;bm_math_op PROC EXPORT FRAME
    ; .xdata for a function's structured exception handling unwind behavior
;.endprolog



;bm_math_op ENDP



bm_nop_op PROC EXPORT FRAME
.endprolog

    mov rax, rcx
    test rax, rax
nopLoopBegin:
    je nopLoopEnd
    dec rax
    jmp nopLoopBegin
nopLoopEnd:
    ret

bm_nop_op ENDP


bm_nopx4_op PROC EXPORT FRAME
.endprolog

    mov rax, rcx
    test rax, rax
nopx4LoopBegin:
    je nopx4LoopEnd
    dec rax
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    jmp nopx4LoopBegin
nopx4LoopEnd:
    ret

bm_nopx4_op ENDP

END