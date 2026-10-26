; Owned x64 arithmetic source. Assemble through the generated .asm wrapper.
; The existing MSVC/MASM toolchain is already used by other project components.
; No copied executable bytes, engine calls, imports, or runtime DLL dependency.
option casemap:none
.const
ALIGN 16
PUBLIC materialTrigReductionPeriodCandidate
; Independently evaluated pi*2^62, rounded by MASM to extended precision.
materialTrigReductionPeriodCandidate REAL10 1.44880389161542456848E19
.code
materialTrigX87Candidate PROC FRAME
    sub rsp,24
    .allocstack 24
    .endprolog
    fnstcw WORD PTR [rsp]
    mov WORD PTR [rsp+2],027fh
    fldcw WORD PTR [rsp+2]
    movss DWORD PTR [rsp+8],xmm0
    fld DWORD PTR [rsp+8]
    test edx,edx
    jnz cosineFirst
    fsin
    jmp checkRange
cosineFirst:
    fcos
checkRange:
    fnstsw ax
    test ax,0400h
    jz finishResult
    fld TBYTE PTR [materialTrigReductionPeriodCandidate]
    fxch st(1)
reduceAgain:
    fprem1
    fnstsw ax
    test ax,0400h
    jnz reduceAgain
    fstp st(1)
    test edx,edx
    jnz cosineReduced
    fsin
    jmp finishResult
cosineReduced:
    fcos
finishResult:
    fstp DWORD PTR [rsp+8]
    fldcw WORD PTR [rsp]
    movss xmm0,DWORD PTR [rsp+8]
    add rsp,24
    ret
materialTrigX87Candidate ENDP

; Test accessor: control word and stack TOP only, not floating status flags.
materialTrigX87EnvironmentCandidate PROC FRAME
    sub rsp,8
    .allocstack 8
    .endprolog
    fnstcw WORD PTR [rsp]
    fnstsw ax
    and eax,03800h
    shl eax,16
    movzx edx,WORD PTR [rsp]
    or eax,edx
    add rsp,8
    ret
materialTrigX87EnvironmentCandidate ENDP
