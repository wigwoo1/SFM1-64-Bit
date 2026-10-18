#define NOMINMAX
#include <windows.h>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>
// Test-only original DLL oracle. The owned editor never links or loads this.
struct Query {std::array<float,3>start{},delta{},origin{},angles{};uint32_t mask=0x4481;};
static_assert(sizeof(Query)==52,"physics query wire");
static size_t active=0;static const char*stage="startup";
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"physics fault stage="<<stage<<" case="<<active<<" code="<<std::hex<<p->ExceptionRecord->ExceptionCode<<" eip="<<p->ContextRecord->Eip<<std::endl;ExitProcess(90);}
static std::vector<uint8_t>read(const char*path){std::ifstream in(path,std::ios::binary);if(!in)throw std::runtime_error("physics input read");return {(std::istreambuf_iterator<char>(in)),{}};}
static uint32_t word(const std::vector<uint8_t>&b,size_t at){if(at>b.size()||b.size()-at<4)throw std::runtime_error("PHY word outside file");uint32_t n;std::memcpy(&n,b.data()+at,4);return n;}
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==4,"test-only x86 physics oracle");if(argc!=7)throw std::runtime_error("dll-directory phy queries results metadata mesh required");AddVectoredExceptionHandler(1,fault);
    const std::string directory=argv[1];if(!SetDllDirectoryA(directory.c_str()))throw std::runtime_error("DLL directory");
    stage="tier0 load";HMODULE tier=LoadLibraryExA((directory+"\\tier0.dll").c_str(),nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);if(!tier)throw std::runtime_error("copied tier0 load failed "+std::to_string(GetLastError()));
    stage="vphysics load";HMODULE module=LoadLibraryExA((directory+"\\vphysics.dll").c_str(),nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);if(!module)throw std::runtime_error("copied vphysics load failed "+std::to_string(GetLastError()));
    using Factory=void*(__cdecl*)(const char*,int*);const auto factory=reinterpret_cast<Factory>(GetProcAddress(module,"CreateInterface"));if(!factory)throw std::runtime_error("physics factory export");int code=-1;stage="factory";void*object=factory("VPhysicsCollision007",&code);if(!object||code!=0)throw std::runtime_error("physics interface missing");const auto table=*reinterpret_cast<uintptr_t**>(object);
    auto rva=[&](uintptr_t address){return address-reinterpret_cast<uintptr_t>(module);};if(rva(table[31])!=0x5e00||rva(table[36])!=0x90a0||rva(table[37])!=0x5eb0)throw std::runtime_error("pinned physics vtable changed");
    auto bytes=read(argv[2]);const auto before=bytes;const uint32_t header=word(bytes,0),solids=word(bytes,8);if(header<16||header>=bytes.size()||!solids||solids>1024||bytes.size()-header>INT32_MAX)throw std::runtime_error("PHY header/solid span");size_t end=header;for(uint32_t i=0;i<solids;++i){const auto size=word(bytes,end);if(size>bytes.size()-end-4||size<8)throw std::runtime_error("PHY solid extent");end+=4+size;}if(end>=bytes.size())throw std::runtime_error("PHY script terminator");
    alignas(16)std::array<uint8_t,32>vcollide{};vcollide.fill(0xa7);using Load=void(__thiscall*)(void*,void*,int,const void*,int,bool);using Unload=void(__thiscall*)(void*,void*);using Trace=void(__thiscall*)(void*,const void*,uint32_t,const void*,const void*,const float*,const float*,void*);
    stage="VCollideLoad";reinterpret_cast<Load>(table[36])(object,vcollide.data()+8,int(solids),bytes.data()+header,int(bytes.size()-header),false);uint16_t loaded=0;std::memcpy(&loaded,vcollide.data()+8,2);loaded&=0x7fff;void**shapes=nullptr;std::memcpy(&shapes,vcollide.data()+12,4);if(loaded!=solids||!shapes||!shapes[0])throw std::runtime_error("original collision shape not loaded");
    if(bytes!=before)throw std::runtime_error("physics loader mutated input copy");
    stage="CreateQueryModel";using CreateQuery=void*(__thiscall*)(void*,void*);using Count=int(__thiscall*)(void*);using Indexed=int(__thiscall*)(void*,int);using Vertices=void(__thiscall*)(void*,int,int,float*);using DestroyQuery=void(__thiscall*)(void*,void*);
    std::ofstream mesh(argv[6],std::ios::binary);if(!mesh)throw std::runtime_error("physics mesh output");mesh.write(reinterpret_cast<const char*>(&loaded),2);
    for(uint16_t s=0;s<loaded;++s){void*query=reinterpret_cast<CreateQuery>(table[43])(object,shapes[s]);if(!query)throw std::runtime_error("query model missing");const auto qt=*reinterpret_cast<uintptr_t**>(query);const int count=reinterpret_cast<Count>(qt[1])(query);if(count<0||count>65536)throw std::runtime_error("convex count");mesh.write(reinterpret_cast<const char*>(&count),4);
        for(int c=0;c<count;++c){const int triangles=reinterpret_cast<Indexed>(qt[2])(query,c),gameData=reinterpret_cast<Indexed>(qt[3])(query,c);if(triangles<0||triangles>65536)throw std::runtime_error("triangle count");mesh.write(reinterpret_cast<const char*>(&gameData),4);mesh.write(reinterpret_cast<const char*>(&triangles),4);for(int t=0;t<triangles;++t){std::array<float,11>v;v.fill(-123456);reinterpret_cast<Vertices>(qt[4])(query,c,t,v.data()+1);if(v.front()!=-123456||v.back()!=-123456)throw std::runtime_error("triangle canary");for(size_t i=1;i<10;++i)if(!std::isfinite(v[i]))throw std::runtime_error("nonfinite original triangle");mesh.write(reinterpret_cast<const char*>(v.data()+1),36);}}
        reinterpret_cast<DestroyQuery>(table[44])(object,query);
    }mesh.close();if(!mesh)throw std::runtime_error("mesh write failed");
    std::ifstream queries(argv[3],std::ios::binary);std::ofstream results(argv[4],std::ios::binary);if(!queries||!results)throw std::runtime_error("physics query IO");Query q;size_t hits=0,inside=0;stage="TraceBox";
    while(queries.read(reinterpret_cast<char*>(&q),sizeof(q))){for(float x:q.start)if(!std::isfinite(x))throw std::runtime_error("nonfinite physics start");for(float x:q.delta)if(!std::isfinite(x))throw std::runtime_error("nonfinite physics delta");for(float x:q.origin)if(!std::isfinite(x))throw std::runtime_error("nonfinite prop origin");for(float x:q.angles)if(!std::isfinite(x))throw std::runtime_error("nonfinite prop angles");const auto old=q;
        alignas(16)std::array<uint8_t,80>ray{};std::memcpy(ray.data(),q.start.data(),12);std::memcpy(ray.data()+16,q.delta.data(),12);const float xy=q.delta[0]*q.delta[0]+q.delta[1]*q.delta[1],squared=xy+q.delta[2]*q.delta[2];ray[68]=1;ray[69]=squared!=0;const auto beforeRay=ray;
        alignas(16)std::array<uint8_t,112>trace{};trace.fill(0xa7);std::memset(trace.data()+16,0,84);reinterpret_cast<Trace>(table[31])(object,ray.data(),q.mask,nullptr,shapes[0],q.origin.data(),q.angles.data(),trace.data()+16);
        if(std::memcmp(&q,&old,sizeof(q))||ray!=beforeRay)throw std::runtime_error("physics trace input mutation");for(size_t i=0;i<16;++i)if(trace[i]!=0xa7)throw std::runtime_error("physics leading canary");for(size_t i=100;i<trace.size();++i)if(trace[i]!=0xa7)throw std::runtime_error("physics trailing canary");
        float fraction;std::memcpy(&fraction,trace.data()+16+44,4);if(!std::isfinite(fraction)||fraction<0||fraction>1)throw std::runtime_error("invalid native trace fraction");hits+=fraction<1;inside+=trace[16+55]!=0;results.write(reinterpret_cast<const char*>(trace.data()+16),60);++active;
    }if(!queries.eof()||queries.gcount()!=0||!active)throw std::runtime_error("physics query extent");results.flush();if(!results)throw std::runtime_error("physics output failed");
    stage="VCollideUnload";reinterpret_cast<Unload>(table[37])(object,vcollide.data()+8);for(size_t i=0;i<8;++i)if(vcollide[i]!=0xa7||vcollide[24+i]!=0xa7)throw std::runtime_error("vcollide canary");if(bytes!=before)throw std::runtime_error("physics input changed");
    std::ofstream meta(argv[5]);meta<<"{\"status\":\"PASS\",\"cases\":"<<active<<",\"fraction_hits\":"<<hits<<",\"start_solid\":"<<inside<<",\"loaded_solids\":"<<loaded<<",\"trace_rva\":\"0x5e00\",\"load_rva\":\"0x90a0\",\"unload_rva\":\"0x5eb0\"}\n";meta.close();if(!meta)throw std::runtime_error("physics metadata output");
    stage="unload modules";FreeLibrary(module);FreeLibrary(tier);SetDllDirectoryA(nullptr);std::cout<<"PASS original physics shape traces="<<active<<" hits="<<hits<<" start-solid="<<inside<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<"physics stage="<<stage<<" "<<e.what()<<'\n';return 1;}}
