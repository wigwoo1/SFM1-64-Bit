#include "source_displacement_face_input_candidate.h"
#include <fstream>
#include <iostream>
#ifdef NATIVE_FACE_INPUT
#define NOMINMAX
#include <windows.h>
#include "native_displacement_face_input_bytes.h"
static size_t activeCase=0;static unsigned activePhase=0;
static LONG CALLBACK reportFault(EXCEPTION_POINTERS*p){std::cerr<<"native fixture fault case="<<activeCase<<" phase="<<activePhase<<" eip="<<std::hex<<p->ContextRecord->Eip<<" ebp="<<p->ContextRecord->Ebp<<" eax="<<p->ContextRecord->Eax<<" ecx="<<p->ContextRecord->Ecx<<" edx="<<p->ContextRecord->Edx<<" address="<<p->ExceptionRecord->ExceptionInformation[1]<<std::endl;ExitProcess(90);}
// Original basic blocks retain their original branches and frame offsets.
// A test-only RET at each exclusive end returns to these supplied frames.
__declspec(naked) static void nativeMap(void*,const void*,uint32_t,uint32_t,void*){__asm{
 push ebp
 mov ebp,esp
 sub esp,1000h
 mov dword ptr [esp],0
 sub esp,1000h
 mov dword ptr [esp],0
 sub esp,900h
 mov dword ptr [esp],0
 push ebx
 push esi
 push edi
 mov eax,dword ptr [ebp+0ch]
 mov dword ptr [ebp-27d8h],eax
 mov eax,dword ptr [ebp+10h]
 mov dword ptr [ebp-27e0h],eax
 mov eax,dword ptr [ebp+18h]
 mov dword ptr [ebp-2800h],eax
 mov esi,dword ptr [ebp+14h]
 xor edx,edx
 call dword ptr [ebp+8]
 pop edi
 pop esi
 pop ebx
 add esp,2900h
 pop ebp
 ret
}}
__declspec(naked) static void nativeQuad(void*,const void*,const void*,const void*,const void*,void*){__asm{
 push ebp
 mov ebp,esp
 sub esp,1000h
 mov dword ptr [esp],0
 sub esp,1000h
 mov dword ptr [esp],0
 sub esp,900h
 mov dword ptr [esp],0
 push ebx
 push esi
 push edi
 mov ecx,dword ptr [ebp+0ch]
 mov dword ptr [ebp-27d8h],ecx
 mov eax,dword ptr [ebp+18h]
 mov dword ptr [ebp-2804h],eax
 mov dword ptr [ebp-27f4h],4
 mov esi,4
 mov edi,dword ptr [ebp+10h]
 mov ebx,dword ptr [ebp+14h]
 call dword ptr [ebp+8]
 lea esi,[ebp-440h]
 mov edi,dword ptr [ebp+1ch]
 mov ecx,12
 rep movsd
 pop edi
 pop esi
 pop ebx
 add esp,2900h
 pop ebp
 ret
}}
#endif
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("input output required");std::ifstream in(argv[1],std::ios::binary);std::ofstream out(argv[2],std::ios::binary);if(!in||!out)throw std::runtime_error("workspace files");
#ifdef NATIVE_FACE_INPUT
 AddVectoredExceptionHandler(1,reportFault);
 auto base=static_cast<uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),0x160000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(base!=reinterpret_cast<void*>(0x10000000))throw std::runtime_error("native base unavailable");
 for(const auto&f:faceInputFragments){std::memcpy(base+f.offset,f.bytes,f.size);base[f.offset+f.size]=0xc3;}
 DWORD old;if(!VirtualProtect(base+0x140000,0x10000,PAGE_EXECUTE_READ,&old))throw std::runtime_error("code protection");FlushInstructionCache(GetCurrentProcess(),base,0x160000);
#endif
 std::array<uint32_t,5>header{};size_t cases=0,quads=0;
 while(in.read(reinterpret_cast<char*>(header.data()),20)){
  auto read=[&](auto&v,size_t count){if(count>1000000)throw std::runtime_error("fixture count");v.resize(count);if(count&&!in.read(reinterpret_cast<char*>(v.data()),std::streamsize(count*sizeof(v[0]))))throw std::runtime_error("truncated fixture");};
  std::vector<SourceDisplacementFaceInputCandidate>faces;std::vector<std::array<float,3>>vertices;std::vector<std::array<uint16_t,2>>edges;std::vector<int32_t>surfedges;read(faces,header[1]);read(vertices,header[2]);read(edges,header[3]);read(surfedges,header[4]);
  std::vector<uint16_t>mapping(header[0],0xffff);
  for(const auto&f:faces){int16_t index;std::memcpy(&index,f.data()+12,2);if(index< -1)throw std::runtime_error("unsafe native fixture label");}
#ifdef NATIVE_FACE_INPUT
  activeCase=cases;activePhase=0;nativeMap(base+0x14c1f9,faces.data(),uint32_t(faces.size()),header[0],mapping.data());
#else
  mapping=sourceDisplacementFaceMapCandidate(faces,header[0]);
#endif
  out.write(reinterpret_cast<const char*>(mapping.data()),mapping.size()*2);
  // Non-quad / missing associations are not sent to the quad routine.
  for(auto id:mapping){if(id==0xffff)continue;if(id>=faces.size())throw std::runtime_error("mapped face outside fixture");int16_t n;std::memcpy(&n,faces[id].data()+8,2);if(n!=4)continue;std::array<std::array<float,3>,4>quad{};
#ifdef NATIVE_FACE_INPUT
   activePhase=1;nativeQuad(base+0x14c42f,faces[id].data(),vertices.data(),edges.data(),surfedges.data(),quad.data());
#else
   quad=sourceDisplacementFaceQuadCandidate(faces[id],vertices,edges,surfedges);
#endif
   out.write(reinterpret_cast<const char*>(quad.data()),48);++quads;
  }++cases;
 }
 if(!in.eof()||in.gcount()!=0||!cases)throw std::runtime_error("input length");out.flush();if(!out)throw std::runtime_error("output failure");
#ifdef NATIVE_FACE_INPUT
 for(const auto&f:faceInputFragments)if(std::memcmp(base+f.offset,f.bytes,f.size))throw std::runtime_error("original block changed");VirtualFree(base,0,MEM_RELEASE);
#endif
 std::cout<<"PASS face-input cases="<<cases<<" quads="<<quads<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
