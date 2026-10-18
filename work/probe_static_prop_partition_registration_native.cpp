// Test-only characterization of original101C4580. The partition CreateHandle
// is a recorder, NOT the real spatial tree. Physics GetAABB executes unchanged.
#define NOMINMAX
#include <Windows.h>
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
#include "native_private_world_image_fixture.h"
#include "native_vphysics_library_fixture.h"
template<class T>static T at(const void*b,size_t p){T v;std::memcpy(&v,static_cast<const uint8_t*>(b)+p,sizeof(v));return v;}
template<class T>static void set(void*b,size_t p,const T&v){std::memcpy(static_cast<uint8_t*>(b)+p,&v,sizeof(v));}
template<class T>static void put(std::ostream&o,const T&v){o.write(reinterpret_cast<const char*>(&v),sizeof(v));}
template<class T>static void get(std::istream&i,T&v){if(!i.read(reinterpret_cast<char*>(&v),sizeof(v)))throw std::runtime_error("truncated registration query");}
struct Query {std::array<float,3>origin{},angles{},mins{},maxs{};uint32_t solid=0,cache=0,handle=0;};
static Query active;
static NativeVphysicsLibraryFixture*physics=nullptr;
static void*loaded=nullptr;static void*empty=nullptr;static void*modelAddress=nullptr;static void*propAddress=nullptr;
static std::array<uint32_t,8>events{};static std::array<float,6>bounds{};static size_t caseIndex=0;
static void* __fastcall cacheGet(void*,void*,uint16_t id){if(id!=7)throw std::runtime_error("cache model handle");++events[4];return active.cache==1?nullptr:active.cache==2?empty:loaded;}
static const char* __fastcall modelName(void*,void*,const void*m){if(m!=modelAddress)throw std::runtime_error("model name identity");++events[6];return "models/controlled_registration_fixture.mdl";}
static bool __cdecl logEnabled(int,int severity){if(severity!=1)throw std::runtime_error("log severity");++events[7];return false;}
static void __fastcall physicsBounds(void*,void*,float*lo,float*hi,void*shape,const float*origin,const float*angles){
    if(std::memcmp(origin,active.origin.data(),12)||std::memcmp(angles,active.angles.data(),12)||shape!=at<void*>(at<void*>(loaded,4),0))throw std::runtime_error("physics bounds binding");
    ++events[5];using F=void(__thiscall*)(void*,float*,float*,void*,const float*,const float*);reinterpret_cast<F>(physics->table[23])(physics->object,lo,hi,shape,origin,angles);
}
static uint16_t __fastcall createHandle(void*,void*,void*entity,int mask,const float*lo,const float*hi){
    if(entity!=propAddress||events[0])throw std::runtime_error("partition entity/multiple inserts");events[0]=1;events[2]=uint32_t(mask);std::memcpy(bounds.data(),lo,12);std::memcpy(bounds.data()+3,hi,12);return uint16_t(active.handle);
}
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"registration fault case="<<caseIndex<<" eip="<<std::hex<<p->ContextRecord->Eip<<'\n';ExitProcess(90);}
static std::vector<uint8_t>read(const char*path){std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("PHY read");return {(std::istreambuf_iterator<char>(f)),{}};}
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==4,"original x86 only");if(argc!=6)throw std::runtime_error("engine library-directory PHY queries output required");AddVectoredExceptionHandler(1,fault);
    NativePrivateWorldImageFixture engine(argv[1]);NativeVphysicsLibraryFixture library(argv[2]);physics=&library;if(library.rva(library.table[23])!=0x5e20)throw std::runtime_error("physics GetAABB ABI");
    const auto phy=read(argv[3]);if(phy.size()<16)throw std::runtime_error("PHY header extent");const uint32_t header=at<uint32_t>(phy.data(),0);if(header<16||header>=phy.size()||at<uint32_t>(phy.data(),8)!=1||phy.size()-header>INT32_MAX)throw std::runtime_error("one-solid valid PHY required");
    alignas(16)std::array<uint8_t,32>vc;vc.fill(0xa7);using Load=void(__thiscall*)(void*,void*,int,const void*,int,bool);using Unload=void(__thiscall*)(void*,void*);reinterpret_cast<Load>(library.table[36])(library.object,vc.data()+8,1,phy.data()+header,int(phy.size()-header),false);if((at<uint16_t>(vc.data(),8)&0x7fff)!=1)throw std::runtime_error("PHY did not load");loaded=vc.data()+8;std::array<uint8_t,16>emptyStorage{};empty=emptyStorage.data();
    std::array<uint8_t,0x138>model{};modelAddress=model.data();set(model.data(),0x110,3u);set(model.data(),0x134,uint16_t(7));std::array<uintptr_t,16>cacheTable{};cacheTable[15]=uintptr_t(&cacheGet);uintptr_t cacheObject=uintptr_t(cacheTable.data());set(engine.base,0x694200,uintptr_t(&cacheObject));
    std::array<uintptr_t,24>physicsTable{};physicsTable[23]=uintptr_t(&physicsBounds);uintptr_t physicsObject=uintptr_t(physicsTable.data());set(engine.base,0x6377fc,uintptr_t(&physicsObject));
    std::array<uintptr_t,5>modelTable{};modelTable[4]=uintptr_t(&modelName);uintptr_t modelObject=uintptr_t(modelTable.data());set(engine.base,0x401484,uintptr_t(&modelObject));set(engine.base,0x31a4ac,uintptr_t(&logEnabled));
    std::array<uintptr_t,1>partitionTable{uintptr_t(&createHandle)};set(engine.base,0x6683c0,uintptr_t(partitionTable.data()));engine.seal();using Register=void(__thiscall*)(void*);const auto registration=reinterpret_cast<Register>(engine.base+0x1c4580);
    std::ifstream in(argv[4],std::ios::binary);std::ofstream out(argv[5],std::ios::binary),direct(std::string(argv[5])+".physics",std::ios::binary);if(!in||!out||!direct)throw std::runtime_error("registration streams");
    while(in.peek()!=std::char_traits<char>::eof()){
        get(in,active.origin);get(in,active.angles);get(in,active.mins);get(in,active.maxs);get(in,active.solid);get(in,active.cache);get(in,active.handle);if((active.solid!=0&&active.solid!=2&&active.solid!=6)||active.cache>2||active.handle>65535)throw std::runtime_error("query contract");for(const auto&v:{active.origin,active.angles,active.mins,active.maxs})for(float x:v)if(!std::isfinite(x))throw std::runtime_error("query nonfinite");for(unsigned j=0;j<3;++j)if(active.mins[j]>active.maxs[j])throw std::runtime_error("query inverted box");
        const auto queryBefore=active;events.fill(0);bounds.fill(0);alignas(16)std::array<uint8_t,232>storage;storage.fill(0xa7);uint8_t*prop=storage.data()+16;std::memset(prop,0,200);propAddress=prop;set(prop,0x28,model.data());set(prop,0x2c,uint16_t(0xffff));prop[0x31]=uint8_t(active.solid);std::memcpy(prop+0x10,active.origin.data(),12);std::memcpy(prop+0x1c,active.angles.data(),12);std::memcpy(model.data()+0x118,active.mins.data(),12);std::memcpy(model.data()+0x124,active.maxs.data(),12);const auto before=storage;const auto modelBefore=model;
        registration(prop);events[1]=prop[0x31];events[3]=at<uint16_t>(prop,0x2c);
        auto allowed=before;std::memcpy(allowed.data()+16+0x2c,prop+0x2c,2);allowed[16+0x31]=prop[0x31];if(storage!=allowed||model!=modelBefore||std::memcmp(&active,&queryBefore,sizeof(active)))throw std::runtime_error("original registration changed unrelated bytes/input");
        for(float x:bounds)if(!std::isfinite(x))throw std::runtime_error("nonfinite result");put(out,events);put(out,bounds);
        // Independent direct invocation validates the caller passes/uses actual
        // physics bounds. It does not independently reconstruct their math.
        std::array<float,6>reference{};if(events[5]){using F=void(__thiscall*)(void*,float*,float*,void*,const float*,const float*);reinterpret_cast<F>(library.table[23])(library.object,reference.data(),reference.data()+3,at<void*>(at<void*>(loaded,4),0),active.origin.data(),active.angles.data());if(reference!=bounds||std::memcmp(reference.data(),bounds.data(),24))throw std::runtime_error("partition physics bounds differ from direct GetAABB");}put(direct,reference);++caseIndex;
    }
    if(!caseIndex)throw std::runtime_error("empty query corpus");out.flush();direct.flush();if(!out||!direct)throw std::runtime_error("output failure");reinterpret_cast<Unload>(library.table[37])(library.object,loaded);for(unsigned i=0;i<8;++i)if(vc[i]!=0xa7||vc[24+i]!=0xa7)throw std::runtime_error("collision storage canary");engine.verify();library.verify();VirtualFree(engine.base,0,MEM_RELEASE);std::cout<<"PASS original registration cases="<<caseIndex<<" unchanged code/input/guards; physics bounds direct-match\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
