#pragma once
// Test-only x86 draw-fragment frame, shared with the earlier origin oracle.
// Supply precisely the local frame/register inputs used by this draw fragment.
// A test RET replaces the first following instruction at 10243A57; no rendering
// or unrelated draw-state setup executes.
__declspec(naked) static void invokeOrigin(void*,void*,void*,void*,float*,uint32_t*){__asm{
    push ebp
    mov ebp,esp
    sub esp,260h
    push ebx
    push esi
    push edi
    mov edi,dword ptr [ebp+0ch]
    mov esi,dword ptr [ebp+10h]
    mov eax,dword ptr [ebp+14h]
    mov dword ptr [ebp-218h],eax
    call dword ptr [ebp+8]
    mov eax,dword ptr [ebp+1ch]
    xor ecx,ecx
    test ebx,ebx
    setne cl
    mov dword ptr [eax],ecx
    test ebx,ebx
    jz noPoint
    mov eax,dword ptr [ebp+18h]
    mov ecx,dword ptr [ebx]
    mov dword ptr [eax],ecx
    mov ecx,dword ptr [ebx+4]
    mov dword ptr [eax+4],ecx
    mov ecx,dword ptr [ebx+8]
    mov dword ptr [eax+8],ecx
noPoint:
    pop edi
    pop esi
    pop ebx
    mov esp,ebp
    pop ebp
    ret
}}
