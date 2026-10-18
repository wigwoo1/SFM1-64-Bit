#include "source_displacement_core_geometry_candidate.h"
#include <cstring>
#include <fstream>
#include <iterator>
#include <iostream>
#ifdef NATIVE_CORE_GEOMETRY
#define NOMINMAX
#include <windows.h>
#include <cstdlib>
struct Allocation{unsigned char*base;size_t size;};static std::vector<Allocation>allocations;
static void* __cdecl allocateFixture(size_t size){if(!size||size>1024*1024)ExitProcess(91);auto p=static_cast<unsigned char*>(std::malloc(size+32));if(!p)ExitProcess(92);std::memset(p,0x5a,size+32);std::memset(p+16,0xa5,size);allocations.push_back({p,size});return p+16;}
static void releaseFixtures(){for(const auto&a:allocations){for(size_t i=0;i<16;++i)if(a.base[i]!=0x5a||a.base[a.size+16+i]!=0x5a)throw std::runtime_error("native allocation guard changed");std::free(a.base);}allocations.clear();}
static LONG CALLBACK reportFault(EXCEPTION_POINTERS*p){std::cerr<<"native geometry fault eip="<<std::hex<<p->ContextRecord->Eip<<" address="<<p->ExceptionRecord->ExceptionInformation[1]<<std::endl;ExitProcess(90);}

static uint8_t* loadImage(const char*path,size_t&imageSize,std::vector<std::pair<size_t,std::vector<uint8_t>>>&codeBefore){
 std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("engine read");std::vector<uint8_t>b((std::istreambuf_iterator<char>(f)),{});if(b.size()<sizeof(IMAGE_DOS_HEADER))throw std::runtime_error("PE header");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(b.data());if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<0||size_t(dos->e_lfanew)+sizeof(IMAGE_NT_HEADERS32)>b.size())throw std::runtime_error("PE offset");auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(b.data()+dos->e_lfanew);if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_I386||nt->OptionalHeader.ImageBase!=0x10000000||nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC)throw std::runtime_error("PE identity");imageSize=nt->OptionalHeader.SizeOfImage;if(imageSize>0x2000000||imageSize<0x150000)throw std::runtime_error("PE image size");
 auto base=static_cast<uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),imageSize,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(base!=reinterpret_cast<void*>(0x10000000))throw std::runtime_error("native base unavailable");
 const auto sections=IMAGE_FIRST_SECTION(nt);if(reinterpret_cast<const uint8_t*>(sections)+nt->FileHeader.NumberOfSections*sizeof(IMAGE_SECTION_HEADER)>b.data()+b.size())throw std::runtime_error("PE section table");
 for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){const auto&s=sections[i];if(s.VirtualAddress>imageSize||s.SizeOfRawData>imageSize-s.VirtualAddress||s.PointerToRawData>b.size()||s.SizeOfRawData>b.size()-s.PointerToRawData)throw std::runtime_error("PE section span");std::memcpy(base+s.VirtualAddress,b.data()+s.PointerToRawData,s.SizeOfRawData);}
 base[0x19d6c0]=0xe9;int32_t jump=int32_t(reinterpret_cast<uintptr_t>(&allocateFixture)-(reinterpret_cast<uintptr_t>(base)+0x19d6c5));std::memcpy(base+0x19d6c1,&jump,4);
 DWORD old;for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){const auto&s=sections[i];if(s.Characteristics&IMAGE_SCN_MEM_EXECUTE){codeBefore.push_back({s.VirtualAddress,std::vector<uint8_t>(base+s.VirtualAddress,base+s.VirtualAddress+s.SizeOfRawData)});if(!VirtualProtect(base+s.VirtualAddress,s.SizeOfRawData,PAGE_EXECUTE_READ,&old))throw std::runtime_error("code protection");}}FlushInstructionCache(GetCurrentProcess(),base,imageSize);return base;
}
#endif
int main(int argc,char**argv){try{
 if(argc!=4)throw std::runtime_error("engine input output required");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("workspace files");
#ifdef NATIVE_CORE_GEOMETRY
 AddVectoredExceptionHandler(1,reportFault);size_t imageSize=0;std::vector<std::pair<size_t,std::vector<uint8_t>>>codeBefore;auto base=loadImage(argv[1],imageSize,codeBefore);
 using Constructor=void*(__thiscall*)(void*);using Initialize=void(__thiscall*)(void*,uint32_t,uint32_t,float,const void*,const void*,uint32_t,const void*);using Select=int32_t(__thiscall*)(void*);using Rotate=void(__thiscall*)(void*);using Create=bool(__thiscall*)(void*);
 auto construct=reinterpret_cast<Constructor>(base+0x142a70);auto initialize=reinterpret_cast<Initialize>(base+0x1428e0);auto select=reinterpret_cast<Select>(base+0x13cfa0);auto rotate=reinterpret_cast<Rotate>(base+0x13d0c0);auto create=reinterpret_cast<Create>(base+0x142180);
#endif
 std::array<uint32_t,19>header{};size_t cases=0;
 while(in.read(reinterpret_cast<char*>(header.data()),76)){
  const uint32_t power=header[0],contents=header[1],encoded=header[2];if(power<1||power>4)throw std::runtime_error("fixture power");const size_t cells=size_t(1)<<power,vc=(cells+1)*(cells+1),tc=2*cells*cells;float elevation;std::memcpy(&elevation,&header[3],4);std::array<float,3>start;std::array<std::array<float,3>,4>corners;std::memcpy(start.data(),header.data()+4,12);std::memcpy(corners.data(),header.data()+7,48);std::vector<std::array<float,5>>raw(vc);std::vector<uint16_t>tags(tc);if(!in.read(reinterpret_cast<char*>(raw.data()),vc*20)||!in.read(reinterpret_cast<char*>(tags.data()),tc*2))throw std::runtime_error("truncated fixture");SourceDisplacementCoreGeometryCandidate result;
#ifdef NATIVE_CORE_GEOMETRY
  std::array<uint8_t,0x380>storage;storage.fill(0x5a);auto object=storage.data()+16;std::memset(object,0,0x350);construct(object);initialize(object,power,encoded,elevation,raw.data(),tags.data(),0,nullptr);
  const uint32_t cornerCount=4;std::memcpy(object+0x14,&cornerCount,4);std::memcpy(object+0x18,corners.data(),48);std::memcpy(object+0x2ec,start.data(),12);std::memcpy(object+0x2cc,&contents,4);const auto selected=select(object+0x10);if(selected<0||selected>3)throw std::runtime_error("native start corner missing");rotate(object+0x10);if(!create(object))throw std::runtime_error("native geometry rejected quad");
  auto word=[&](size_t at){uint32_t v;std::memcpy(&v,object+at,4);return v;};result.power=word(0xc);result.contents=word(0x2cc);result.surfaceFlags=word(0x2c8);std::memcpy(result.corners.data(),object+0x18,48);result.positions.resize(vc);result.alphas.resize(vc);const auto vertices=reinterpret_cast<const uint8_t*>(word(0x2f8));for(size_t i=0;i<vc;++i){std::memcpy(result.positions[i].data(),vertices+224*i+40,12);std::memcpy(&result.alphas[i],vertices+224*i+0x8c,4);}result.triangles.resize(tc);std::memcpy(result.triangles.data(),reinterpret_cast<const void*>(word(0x2fc)),tc*8);
  for(size_t i=0;i<16;++i)if(storage[i]!=0x5a)throw std::runtime_error("object front guard");for(size_t i=0x360;i<storage.size();++i)if(storage[i]!=0x5a)throw std::runtime_error("object back guard");releaseFixtures();
#else
  (void)elevation;result=sourceDisplacementCoreGeometryCandidate(power,contents,encoded,corners,start,raw,tags);
#endif
  out.write(reinterpret_cast<const char*>(&result.power),4);out.write(reinterpret_cast<const char*>(&result.contents),4);out.write(reinterpret_cast<const char*>(&result.surfaceFlags),4);out.write(reinterpret_cast<const char*>(result.corners.data()),48);for(size_t i=0;i<vc;++i){out.write(reinterpret_cast<const char*>(result.positions[i].data()),12);out.write(reinterpret_cast<const char*>(&result.alphas[i]),4);}out.write(reinterpret_cast<const char*>(result.triangles.data()),tc*8);++cases;
 }
 if(!in.eof()||in.gcount()!=0||!cases)throw std::runtime_error("input length");out.flush();if(!out)throw std::runtime_error("output failure");
#ifdef NATIVE_CORE_GEOMETRY
 for(const auto&part:codeBefore)if(std::memcmp(base+part.first,part.second.data(),part.second.size()))throw std::runtime_error("private original text changed");VirtualFree(base,0,MEM_RELEASE);
#endif
 std::cout<<"PASS complete original creation collision output cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
