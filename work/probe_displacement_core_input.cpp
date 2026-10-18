#include "source_displacement_core_input_candidate.h"
#include <cstring>
#include <fstream>
#include <iostream>
#ifdef NATIVE_CORE_INPUT
#define NOMINMAX
#include <windows.h>
#include <cstdlib>
#include "native_displacement_core_input_bytes.h"
struct Allocation{unsigned char*base;size_t size;};static std::vector<Allocation>allocations;
static void* __cdecl allocateFixture(size_t size){if(!size||size>1024*1024)ExitProcess(91);auto p=static_cast<unsigned char*>(std::malloc(size+32));if(!p)ExitProcess(92);std::memset(p,0x5a,size+32);std::memset(p+16,0xa5,size);allocations.push_back({p,size});return p+16;}
static void releaseFixtures(){for(const auto&a:allocations){for(size_t i=0;i<16;++i)if(a.base[i]!=0x5a||a.base[a.size+16+i]!=0x5a)throw std::runtime_error("native allocation guard changed");std::free(a.base);}allocations.clear();}
static LONG CALLBACK reportFault(EXCEPTION_POINTERS*p){std::cerr<<"native CoreDisp fault eip="<<std::hex<<p->ContextRecord->Eip<<" address="<<p->ExceptionRecord->ExceptionInformation[1]<<std::endl;ExitProcess(90);}
#endif
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("input output required");std::ifstream in(argv[1],std::ios::binary);std::ofstream out(argv[2],std::ios::binary);if(!in||!out)throw std::runtime_error("workspace files");
#ifdef NATIVE_CORE_INPUT
 AddVectoredExceptionHandler(1,reportFault);auto base=static_cast<uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),0x420000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(base!=reinterpret_cast<void*>(0x10000000))throw std::runtime_error("native base unavailable");for(const auto&f:coreInputFragments)std::memcpy(base+f.offset,f.bytes,f.size);
 // Only the allocation dependency is controlled. All initializer/getter/
 // constructor/default/copy routines below execute their original bytes.
 base[0x19d6c0]=0xe9;int32_t jump=int32_t(reinterpret_cast<uintptr_t>(&allocateFixture)-(reinterpret_cast<uintptr_t>(base)+0x19d6c5));std::memcpy(base+0x19d6c1,&jump,4);
 DWORD old;for(size_t at:{size_t(0),size_t(0xb0000),size_t(0xc0000),size_t(0x130000),size_t(0x140000),size_t(0x190000),size_t(0x2d0000)})if(!VirtualProtect(base+at,0x10000,PAGE_EXECUTE_READ,&old))throw std::runtime_error("protection failed");FlushInstructionCache(GetCurrentProcess(),base,0x420000);
 using Initialize=void(__thiscall*)(void*,uint32_t,uint32_t,float,const void*,const void*,uint32_t,const void*);auto initialize=reinterpret_cast<Initialize>(base+0x1428e0);
#endif
 std::array<uint32_t,6>header{};size_t cases=0;
 while(in.read(reinterpret_cast<char*>(header.data()),24)){
  const uint32_t power=header[0];if(power<1||power>4||header[4]>1)throw std::runtime_error("fixture input");const size_t cells=size_t(1)<<power,vc=(cells+1)*(cells+1),tc=2*cells*cells;
  float elevation;std::memcpy(&elevation,&header[5],4);std::vector<std::array<float,5>>raw(vc);std::vector<uint16_t>tags(tc);std::vector<std::array<float,20>>extra(header[4]?vc:0);
  if(!in.read(reinterpret_cast<char*>(raw.data()),vc*20)||!in.read(reinterpret_cast<char*>(tags.data()),tc*2)||(!extra.empty()&&!in.read(reinterpret_cast<char*>(extra.data()),vc*80)))throw std::runtime_error("truncated fixture");
  SourceDisplacementCoreInputCandidate result;uint32_t allocationCount=4;std::array<uint8_t,0x310>object{};const float retained=42.25f;std::memcpy(object.data()+0x2c8,&header[3],4);std::memcpy(object.data()+0x2e8,&retained,4);
#ifdef NATIVE_CORE_INPUT
  initialize(object.data(),power,header[1],elevation,raw.data(),tags.data(),header[2],extra.empty()?nullptr:extra.data());
  auto word=[&](size_t at){uint32_t v;std::memcpy(&v,object.data()+at,4);return v;};result.power=word(0xc);result.surfaceFlags=word(0x2c8);result.flags=word(0x300);result.vertices.resize(vc);std::memcpy(result.vertices.data(),reinterpret_cast<const void*>(word(0x2f8)),vc*224);result.zeroIndices.resize(vc*6);std::memcpy(result.zeroIndices.data(),reinterpret_cast<const void*>(word(0x308)),vc*12);result.triangleTags.resize(tc);const auto tri=reinterpret_cast<const uint8_t*>(word(0x2fc));for(size_t i=0;i<tc;++i)std::memcpy(&result.triangleTags[i],tri+i*8+6,2);
  allocationCount=uint32_t(allocations.size());if(allocationCount!=4||allocations[0].size!=vc*224||allocations[1].size!=vc*12||allocations[2].size!=(cells*cells/3)*512||allocations[3].size!=tc*8)throw std::runtime_error("native allocation sizes");
  for(size_t at:{size_t(4),size_t(0x2f8),size_t(0x2fc),size_t(0x308)}){uint32_t symbol=1;std::memcpy(object.data()+at,&symbol,4);}releaseFixtures();
#else
  (void)elevation;result=sourceDisplacementCoreInputCandidate(power,header[1],header[2],header[3],raw,tags,extra);
  std::memcpy(object.data()+0xc,&result.power,4);std::memcpy(object.data()+0x2c8,&result.surfaceFlags,4);std::memcpy(object.data()+0x300,&result.flags,4);for(size_t at:{size_t(4),size_t(0x2f8),size_t(0x2fc),size_t(0x308)}){uint32_t symbol=1;std::memcpy(object.data()+at,&symbol,4);}
#endif
  out.write(reinterpret_cast<const char*>(object.data()),object.size());out.write(reinterpret_cast<const char*>(&allocationCount),4);out.write(reinterpret_cast<const char*>(result.vertices.data()),vc*224);out.write(reinterpret_cast<const char*>(result.zeroIndices.data()),vc*12);out.write(reinterpret_cast<const char*>(result.triangleTags.data()),tc*2);++cases;
 }
 if(!in.eof()||in.gcount()!=0||!cases)throw std::runtime_error("input length");out.flush();if(!out)throw std::runtime_error("output failure");
#ifdef NATIVE_CORE_INPUT
 for(const auto&f:coreInputFragments)if(std::memcmp(base+f.offset,f.bytes,f.size))throw std::runtime_error("original code changed");VirtualFree(base,0,MEM_RELEASE);
#endif
 std::cout<<"PASS CoreDisp initialization cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
