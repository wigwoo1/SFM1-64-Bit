#include "physics_leaf_test_fixture.h"
#include <fstream>
#include <iostream>
#ifdef NATIVE_PHYSICS_LEAF_HIT
#define NOMINMAX
#include <windows.h>
#include "native_vphysics_library_fixture.h"
#include "native_physics_search_fixture.h"
// Test-only trampoline for original2A389..2A4E6. The inner frame exactly
// matches original2A340's saved EBP/EBX/ESI/EDI and its ret4 argument. This
// directly executes the intact hit policy, without claiming a whole leaf call.
__declspec(naked)static void __cdecl invokeHitNative(void*,void*,uint32_t){__asm{
    push ebp
    mov ebp,esp
    push ebx
    push esi
    push edi
    push 0
    call inner
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret
inner:
    push ebp
    mov ebp,esp
    push ebx
    push esi
    push edi
    mov eax,dword ptr [ebp]
    mov esi,dword ptr [eax+0ch]
    mov ebx,dword ptr [eax+10h]
    mov eax,dword ptr [eax+8]
    jmp eax
}}
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"hit policy fault eip="<<std::hex<<p->ContextRecord->Eip<<std::endl;ExitProcess(90);}
#endif
int main(int argc,char**argv){try{
    if(argc!=4)throw std::runtime_error("DLL directory input output required");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("hit fixture streams");
#ifdef NATIVE_PHYSICS_LEAF_HIT
    static_assert(sizeof(void*)==4,"original x86");AddVectoredExceptionHandler(1,fault);NativeVphysicsLibraryFixture image(argv[1]);const auto defaultSurface=searchDefaultSurfaceNative(image);
#else
    static_assert(sizeof(void*)==8,"owned AMD64");
#endif
    size_t cases=0;while(in.peek()!=std::char_traits<char>::eof()){SourcePhysicsLeafStateCandidate s;uint32_t contents;leafReadState(in,s);supportGet(in,contents);const auto before=leafStateBytes(s);
#ifdef NATIVE_PHYSICS_LEAF_HIT
        NativeSupportAlignedBytesFixture self(0xe4),ray(64),trace(84);searchWriteNativeTrace(self.data()+4,s.search.trace,defaultSurface);std::memcpy(self.data()+0x58,s.search.direction.data(),12);supportWriteNative(self.data(),0x74,s.search.distance);supportWriteNative(self.data(),0x78,s.nearest);supportWriteNative(self.data(),0x7c,s.tree.boxRadius);supportWriteNative(self.data(),0x80,s.tolerance);supportWriteNative(self.data(),0x6c,ray.data());supportWriteNative(self.data(),0x70,trace.data());searchWriteNativeTrace(trace.data(),s.trace,defaultSurface);std::memcpy(ray.data(),s.ray.data(),64);std::memcpy(self.data()+0xa0,s.tree.midpoint.data(),12);std::memcpy(self.data()+0xb0,s.tree.start.data(),12);std::memcpy(self.data()+0xc0,s.tree.direction.data(),12);std::memcpy(self.data()+0xd0,s.localDelta.data(),12);supportWriteNative(self.data(),0xe0,s.tree.length);const std::vector<uint8_t>oldSelf(self.data(),self.data()+self.size);invokeHitNative(image.address(0x2a389),self.data(),contents);
        for(size_t j=0;j<self.size;++j)if(!((j>=0x78&&j<0x7c)||(j>=0xa0&&j<0xac)||(j>=0xe0&&j<0xe4))&&self.data()[j]!=oldSelf[j])throw std::runtime_error("hit policy changed unrelated object field");s.search=searchReadNativeState(self.data(),defaultSurface);s.trace=searchReadNativeTrace(trace.data(),defaultSurface);std::memcpy(s.ray.data(),ray.data(),64);std::memcpy(s.tree.midpoint.data(),self.data()+0xa0,12);std::memcpy(&s.tree.length,self.data()+0xe0,4);std::memcpy(&s.nearest,self.data()+0x78,4);self.verify();ray.verify();trace.verify();
#else
        sourcePhysicsLeafHitCandidate(s,contents);
#endif
        leafWriteState(out,s);++cases;
    }if(!cases)throw std::runtime_error("no hit cases");out.flush();if(!out)throw std::runtime_error("hit output");
#ifdef NATIVE_PHYSICS_LEAF_HIT
    image.verify();
#endif
    std::cout<<"PASS direct hit policy cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
