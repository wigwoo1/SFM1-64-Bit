#include "source_model_lighting_point_candidate.h"
#include <cstring>
#include <fstream>
#include <iterator>
#include <iostream>
#include <vector>
#ifdef NATIVE_MODEL_LIGHTING_POINT
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
static RecoveredMatrix3x4 attachmentMatrix{};static bool attachmentSuccess=false;static uint32_t attachmentCalls=0;static int32_t observedAttachment=0;
static bool __fastcall attachmentHook(void*,void*,int32_t index,void*out){++attachmentCalls;observedAttachment=index;std::memcpy(out,attachmentMatrix.data(),48);return attachmentSuccess;}
__declspec(naked) static void invokeLightingOrigin(void*,const void*,const float*,float*){__asm{
 push ebp
 mov ebp,esp
 sub esp,180h
 push ebx
 push esi
 push edi
 mov eax,dword ptr [ebp+0ch]
 mov dword ptr [ebp-140h],eax
 mov ebx,dword ptr [ebp+10h]
 call dword ptr [ebp+8]
 mov eax,dword ptr [ebp+14h]
 mov ecx,dword ptr [ebp-150h]
 mov dword ptr [eax],ecx
 mov ecx,dword ptr [ebp-14ch]
 mov dword ptr [eax+4],ecx
 mov ecx,dword ptr [ebp-148h]
 mov dword ptr [eax+8],ecx
 pop edi
 pop esi
 pop ebx
 mov esp,ebp
 pop ebp
 ret
}}
static size_t activeCase=0;static LONG CALLBACK fault(EXCEPTION_POINTERS*p){std::cerr<<"native lighting point fault case="<<activeCase<<" eip="<<std::hex<<p->ContextRecord->Eip<<" address="<<p->ExceptionRecord->ExceptionInformation[1]<<std::endl;ExitProcess(90);}
#endif
int main(int argc,char**argv){try{
 if(argc!=4)throw std::runtime_error("engine input output required");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("workspace files");
#ifdef NATIVE_MODEL_LIGHTING_POINT
 AddVectoredExceptionHandler(1,fault);NativePrivateWorldImageFixture image(argv[1]);auto base=image.base;base[0xd57d3]=0xc3;image.seal();std::array<uintptr_t,35>vtable{};vtable[34]=reinterpret_cast<uintptr_t>(&attachmentHook);uintptr_t object=reinterpret_cast<uintptr_t>(vtable.data());using Illumination=void(__cdecl*)(void*,const void*,const float*,float*);
#endif
 std::array<uint32_t,44>q{};size_t count=0;while(in.read(reinterpret_cast<char*>(q.data()),176)){
  if(q[0]&~15u)throw std::runtime_error("fixture flags");int32_t attachmentIndex;std::memcpy(&attachmentIndex,&q[1],4);std::array<float,3>local{},explicitPoint{},defaultPoint{},finalPoint{};RecoveredMatrix3x4 model{},attachment{},offset{};std::memcpy(local.data(),q.data()+2,12);std::memcpy(model.data(),q.data()+5,48);std::memcpy(attachment.data(),q.data()+17,48);std::memcpy(offset.data(),q.data()+29,48);std::memcpy(explicitPoint.data(),q.data()+41,12);uint32_t calls=0;int32_t observed=0;
#ifdef NATIVE_MODEL_LIGHTING_POINT
  activeCase=count;attachmentMatrix=attachment;attachmentSuccess=(q[0]&2)!=0;attachmentCalls=0;observedAttachment=0;std::array<uint8_t,0x200>header{};std::memcpy(header.data()+0x5c,local.data(),12);if(q[0]&1){uint32_t extra=0x1a0;std::memcpy(header.data()+0x190,&extra,4);std::memcpy(header.data()+0x1a8,&attachmentIndex,4);}reinterpret_cast<Illumination>(base+0xd2780)(&object,header.data(),model[0].data(),defaultPoint.data());
  std::array<uint8_t,0x44>params{};auto pointer=[&](size_t at,const void*p){uint32_t address=reinterpret_cast<uint32_t>(p);std::memcpy(params.data()+at,&address,4);};if(q[0]&4)pointer(0x24,offset.data());if(q[0]&8)pointer(0x28,explicitPoint.data());invokeLightingOrigin(base+0xd5760,params.data(),defaultPoint.data(),finalPoint.data());calls=attachmentCalls;observed=observedAttachment;
#else
  defaultPoint=sourceModelIlluminationPointCandidate(local,model,(q[0]&1)?attachmentIndex:0,[&](int32_t index,auto&result){++calls;observed=index;result=attachment;return (q[0]&2)!=0;});finalPoint=sourceModelLightingOriginCandidate(defaultPoint,(q[0]&4)?&offset:nullptr,(q[0]&8)?&explicitPoint:nullptr);
#endif
  out.write(reinterpret_cast<const char*>(defaultPoint.data()),12);out.write(reinterpret_cast<const char*>(finalPoint.data()),12);out.write(reinterpret_cast<const char*>(&calls),4);out.write(reinterpret_cast<const char*>(&observed),4);++count;
 }if(!in.eof()||in.gcount()!=0||!count)throw std::runtime_error("input length");out.flush();if(!out)throw std::runtime_error("output failure");
#ifdef NATIVE_MODEL_LIGHTING_POINT
 image.verify();VirtualFree(base,0,MEM_RELEASE);
#endif
 std::cout<<"PASS model illumination/origin cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
