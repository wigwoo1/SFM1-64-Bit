; Owned double-input/double-output hardware path for legacy cosine math.
; Uses the already independently generated extended reduction period.
; No executable bytes, engine calls, or external DLL dependencies.
.code
materialCosineDoubleX87Candidate PROC FRAME
    sub rsp,24
    .allocstack 24
    .endprolog
    fnstcw WORD PTR [rsp]
    mov WORD PTR [rsp+2],027fh
    fldcw WORD PTR [rsp+2]
    movsd QWORD PTR [rsp+8],xmm0
    fld QWORD PTR [rsp+8]
    fcos
    fnstsw ax
    test ax,0400h
    jz cosineDoubleResult
    fld TBYTE PTR [materialTrigReductionPeriodCandidate]
    fxch st(1)
cosineDoubleReduce:
    fprem1
    fnstsw ax
    test ax,0400h
    jnz cosineDoubleReduce
    fstp st(1)
    fcos
cosineDoubleResult:
    fstp QWORD PTR [rsp+8]
    fldcw WORD PTR [rsp]
    movsd xmm0,QWORD PTR [rsp+8]
    add rsp,24
    ret
materialCosineDoubleX87Candidate ENDP
