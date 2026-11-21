option casemap:none
.code
sfm64OriginalStyleSinCos PROC
    fld DWORD PTR [rcx]
    fsincos
    fstp DWORD PTR [r8]
    fstp DWORD PTR [rdx]
    ret
sfm64OriginalStyleSinCos ENDP
END
