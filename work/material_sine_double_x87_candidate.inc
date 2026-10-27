; Owned double-input/double-output hardware path for legacy Sine math.
; Uses the already independently generated extended reduction period.
; No executable bytes, engine calls, or external DLL dependencies.
.code
materialSineDoubleX87Candidate PROC FRAME
    sub rsp,24
    .allocstack 24
    .endprolog
    fnstcw WORD PTR [rsp]
    mov WORD PTR [rsp+2],027fh
    fldcw WORD PTR [rsp+2]
    movsd QWORD PTR [rsp+8],xmm0
    fld QWORD PTR [rsp+8]
    fsin
    fnstsw ax
    test ax,0400h
    jz sineDoubleResult
    fld TBYTE PTR [materialTrigReductionPeriodCandidate]
    fxch st(1)
sineDoubleReduce:
    fprem1
    fnstsw ax
    test ax,0400h
    jnz sineDoubleReduce
    fstp st(1)
    fsin
sineDoubleResult:
    fstp QWORD PTR [rsp+8]
    fldcw WORD PTR [rsp]
    movsd xmm0,QWORD PTR [rsp+8]
    add rsp,24
    ret
materialSineDoubleX87Candidate ENDP
