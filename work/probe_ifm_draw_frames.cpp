#include "ifm_draw_frames_candidate.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
struct FrameQuery {int32_t model=1,mode=0,count=1,child=1;RecoveredMatrix3x4 root{},childWorld{};IfmBoundsCandidate bounds{};};
static_assert(sizeof(FrameQuery)==136,"query wire");
struct FrameResult {IfmBoundsWrapperFramesCandidate frames{},atBounds{};std::array<uint32_t,10>events{};};
static_assert(sizeof(FrameResult)==136,"result wire");
static FrameResult current;static FrameQuery query;
static void event(uint32_t value){auto&n=current.events[0];if(n>=9)throw std::runtime_error("event limit");current.events[++n]=value;}
#ifdef NATIVE_IFM_DRAW_FRAMES
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
static void*__fastcall resolve(void*,void*,uint32_t){event(1);return query.model?reinterpret_cast<void*>(1):nullptr;}
static void __fastcall world(void*node,void*,void*out){const bool child=node==reinterpret_cast<void*>(2);event(child?5:2);std::memcpy(out,(child?query.childWorld:query.root).data(),48);}
static int32_t __fastcall count(void*,void*){event(3);return query.count;}
static void*__fastcall child(void*,void*,uint32_t index){if(index)throw std::runtime_error("child index");event(4);return query.child?reinterpret_cast<void*>(2):nullptr;}
static void __fastcall bounds(void*object,void*,void*lo,void*hi){event(6);std::memcpy(&current.atBounds,static_cast<uint8_t*>(object)+0x38,48);std::memcpy(lo,query.bounds[0].data(),12);std::memcpy(hi,query.bounds[1].data(),12);}
static void __fastcall notify(void*,void*,uint32_t handle){if(handle!=0xffff)throw std::runtime_error("spatial fixture handle");event(7);}
static void redirect(uint8_t*base,size_t at,void*target){auto*p=base+at;*p=0xe9;const int32_t d=int32_t(reinterpret_cast<intptr_t>(target)-reinterpret_cast<intptr_t>(p)-5);std::memcpy(p+1,&d,4);}
#endif
int main(int argc,char**argv){try{
    if(argc!=5||std::ifstream(argv[4],std::ios::binary).good())throw std::runtime_error("original.dll frames|decompose queries fresh-output required");
    const std::string operation=argv[2];if(operation!="frames"&&operation!="decompose")throw std::runtime_error("operation");
    std::ifstream in(argv[3],std::ios::binary);std::ofstream out(argv[4],std::ios::binary);if(!in||!out)throw std::runtime_error("frame IO");
#ifdef NATIVE_IFM_DRAW_FRAMES
    static_assert(sizeof(void*)==4,"private reference x86");NativePrivateWorldImageFixture image(argv[1]);
    redirect(image.base,0x4c5870,reinterpret_cast<void*>(&world));redirect(image.base,0x4c2b00,reinterpret_cast<void*>(&count));redirect(image.base,0x4c2e70,reinterpret_cast<void*>(&child));redirect(image.base,0x23f310,reinterpret_cast<void*>(&bounds));
    std::array<uintptr_t,19>dmTable{};dmTable[18]=reinterpret_cast<uintptr_t>(&resolve);uintptr_t dm=reinterpret_cast<uintptr_t>(dmTable.data());*reinterpret_cast<uintptr_t*>(image.base+0xe9a51c)=reinterpret_cast<uintptr_t>(&dm);
    std::array<uintptr_t,32>spatialTable{};spatialTable[31]=reinterpret_cast<uintptr_t>(&notify);uintptr_t spatial=reinterpret_cast<uintptr_t>(spatialTable.data());*reinterpret_cast<uintptr_t*>(image.base+0xfd43f4)=reinterpret_cast<uintptr_t>(&spatial);
    std::array<uint8_t,0x34>mode{};*reinterpret_cast<uintptr_t*>(image.base+0xf31774)=reinterpret_cast<uintptr_t>(mode.data());image.seal();
#else
    static_assert(sizeof(void*)==8,"owned x64");
#endif
    size_t cases=0;
    if(operation=="decompose"){
        RecoveredMatrix3x4 matrix{};
        while(in.read(reinterpret_cast<char*>(matrix.data()),48)){
            const auto before=matrix;std::array<uint32_t,9>guard{};guard.front()=guard.back()=0x717ac921;
#ifdef NATIVE_IFM_DRAW_FRAMES
            using Decompose=void(__cdecl*)(const void*,void*,void*,void*);reinterpret_cast<Decompose>(image.base+0x23ba60)(matrix.data(),guard.data()+1,guard.data()+4,guard.data()+7);
#else
            const auto value=ifmDecomposeFrameCandidate(matrix);std::memcpy(guard.data()+1,value.angles.data(),12);std::memcpy(guard.data()+4,value.position.data(),12);std::memcpy(guard.data()+7,&value.scale,4);
#endif
            if(std::memcmp(before.data(),matrix.data(),48)||guard.front()!=0x717ac921||guard.back()!=0x717ac921)throw std::runtime_error("decompose mutation/canary");out.write(reinterpret_cast<const char*>(guard.data()+1),28);++cases;
        }
    }else while(in.read(reinterpret_cast<char*>(&query),sizeof(query))){
        current={};if((query.model!=0&&query.model!=1)||(query.child!=0&&query.child!=1))throw std::runtime_error("query flags");
#ifdef NATIVE_IFM_DRAW_FRAMES
        std::memcpy(mode.data()+0x30,&query.mode,4);std::array<uint8_t,0x90>object{};uint16_t handle=0xffff;std::memcpy(object.data()+0x82,&handle,2);const auto before=object;const auto input=query;
        using Update=void(__thiscall*)(void*);reinterpret_cast<Update>(image.base+0x2402c0)(object.data());
        std::memcpy(&current.frames,object.data()+0x38,48);auto expected=before;std::memcpy(expected.data()+0x38,&current.frames,48);
        if(expected!=object||std::memcmp(&input,&query,sizeof(query)))throw std::runtime_error("frame mutation outside fields");
#else
        struct Services {
            std::optional<int>resolveDag(){event(1);return query.model?std::optional<int>(1):std::nullopt;}
            auto world(int node){event(node==2?5:2);return node==2?query.childWorld:query.root;}
            int32_t childCount(int){event(3);return query.count;}
            std::optional<int>typedChildZero(int){event(4);return query.child?std::optional<int>(2):std::nullopt;}
            auto innerBounds(const IfmBoundsWrapperFramesCandidate&frames){event(6);current.atBounds=frames;return query.bounds;}
        }services;
        current.frames=ifmDrawFramesCandidate(query.mode,services);event(7); // spatial notification is fixture-only
#endif
        out.write(reinterpret_cast<const char*>(&current),sizeof(current));++cases;
    }
    if(!in.eof()||in.gcount()||!cases)throw std::runtime_error("frame query shape");out.flush();if(!out)throw std::runtime_error("frame output");
#ifdef NATIVE_IFM_DRAW_FRAMES
    image.verify();VirtualFree(image.base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS "<<operation<<" cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
