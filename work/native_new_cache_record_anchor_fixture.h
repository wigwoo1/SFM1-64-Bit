#pragma once
// Oracle-only frame for original 1019791B..101979C5. All surrounding cache
// allocation/key lookup/budget logic is outside this fragment comparison.
__declspec(naked) static void invokeNewCacheRecordAnchor(void*,void*,const float*,int32_t,const int32_t*){__asm{
 push ebp
 mov ebp,esp
 sub esp,0b0h
 push ebx
 push esi
 push edi
 mov eax,dword ptr [ebp+10h]
 mov dword ptr [ebp-0a0h],eax
 lea ecx,[eax+4]
 mov dword ptr [ebp-0ach],ecx
 lea edx,[eax+8]
 mov dword ptr [ebp-0a8h],edx
 mov ecx,dword ptr [ebp+14h]
 mov dword ptr [ebp-08ch],ecx
 mov esi,dword ptr [ebp+18h]
 mov ebx,dword ptr [esi]
 mov ecx,dword ptr [esi+4]
 mov edx,dword ptr [esi+8]
 mov eax,dword ptr [ebp+0ch]
 call dword ptr [ebp+8]
 pop edi
 pop esi
 pop ebx
 mov esp,ebp
 pop ebp
 ret
}}
