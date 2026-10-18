#pragma once
// Test-only usercall: ECX=box A, EDX=simplex, EDI=trace; stack ray,shape B.
inline void contactPlaneInvokeNative(void*target,void*aObject,const void*simplexObject,
    const void*rayObject,void*bObject,void*outTrace){
    __asm{push edi
        mov ecx,aObject
        mov edx,simplexObject
        mov edi,outTrace
        push bObject
        push rayObject
        call target
        add esp,8
        pop edi}
}
