; Authored x64 arithmetic, no original bytes or runtime engine dependency.
; Match original GetFloat1 f32 store, GetFloat2 x87 return, FMUL and f32 store.
option casemap:none
.code
materialMultiplyScalarX87Candidate PROC FRAME
    sub rsp,24
    .allocstack 24
    .endprolog
    fnstcw WORD PTR [rsp]
    mov WORD PTR [rsp+2],027fh
    fldcw WORD PTR [rsp+2]
    mov DWORD PTR [rsp+8],ecx
    mov DWORD PTR [rsp+12],edx
    fld DWORD PTR [rsp+8]
    fstp DWORD PTR [rsp+8]
    fld DWORD PTR [rsp+12]
    fmul DWORD PTR [rsp+8]
    fstp DWORD PTR [rsp+16]
    fldcw WORD PTR [rsp]
    mov eax,DWORD PTR [rsp+16]
    add rsp,24
    ret
materialMultiplyScalarX87Candidate ENDP
