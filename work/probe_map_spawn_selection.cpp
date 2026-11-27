#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <utility>
#include <vector>
#include "source_map_spawn_selection_candidate.h"

void require(bool v,const char* s) { if(!v) throw std::runtime_error(s); }
struct Query { int team,cached,choice,world; bool success; } query{};
std::vector<unsigned> events;
uintptr_t actor=0,hint=0,fresh=0,world=0,network=0;
unsigned selectedId=0;
uint32_t cachedHandle=0;
uintptr_t typed=0;
bool warningTeam=false;
constexpr uint32_t freshHandle=0x30002;
uint32_t initialHandle() { return query.cached==0?UINT32_MAX:0x20001; }
uintptr_t initialHint() { return query.cached==1?hint:0; }
unsigned id(uintptr_t value) { if(!value) return 0; if(value==hint) return 1; if(value==fresh) return 2; if(value==world) return 3; throw std::runtime_error("unexpected selected entity"); }
uintptr_t worldLookup() { if(query.world==0 || query.world==4) events.push_back(3); return query.world==0?world:0; }
bool choose(uintptr_t& value) { events.push_back(1); if(query.choice==0) value=0; else if(query.choice==1) value=fresh; return query.success; }
uintptr_t cast(uintptr_t value) { events.push_back(2); return value?7:0; }
void warn(bool team) { events.push_back(4); warningTeam=team; }

#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"
template<class T> T& at(uintptr_t p) { return *reinterpret_cast<T*>(p); }
uint32_t handleValue=freshHandle;
uint32_t* __fastcall getHandle(void* self,void*) {
    const auto owner=reinterpret_cast<uintptr_t>(self); require(owner==hint || owner==fresh,"selected handle owner");
    handleValue=owner==hint?0x20001:freshHandle; return &handleValue;
}
void* __fastcall worldEntity(void* self,void*) { require(reinterpret_cast<uintptr_t>(self)==network,"world networkable owner"); return reinterpret_cast<void*>(worldLookup()); }
void __cdecl warning(const char*,const char* classname) {
    require(std::strcmp(classname,query.team>=2?"info_player_teamspawn":"")==0,"warning classname"); warn(query.team>=2);
}
void boundary(unsigned slot,CONTEXT& c) {
    using O=PrivateX86BoundaryObserverFixture;
    if(slot==0) {
        require(c.Ecx==actor && std::strcmp(reinterpret_cast<const char*>(at<uintptr_t>(c.Esp+4)),"info_player_teamspawn")==0,"spawn lookup arguments");
        auto& entity=at<uintptr_t>(at<uintptr_t>(c.Esp+8)); require(entity==initialHint(),"spawn lookup hint");
        const bool success=choose(entity); O::returnCdecl(c,success); c.Esp+=8;
    } else {
        require(slot==1,"unexpected spawn service"); const auto value=at<uintptr_t>(c.Esp+4);
        require(at<int>(c.Esp+8)==0 && at<uintptr_t>(c.Esp+12)==0x1057c17c &&
            at<uintptr_t>(c.Esp+16)==0x105eb758 && at<int>(c.Esp+20)==0,"spawn type cast arguments");
        O::returnCdecl(c,uint32_t(cast(value)));
    }
}
void run() {
    alignas(4) std::array<uint8_t,0x1810> bytes{};
    std::array<uintptr_t,3> entityTable{};
    std::array<uintptr_t,6> networkTable{};
    uintptr_t hintObject=reinterpret_cast<uintptr_t>(entityTable.data()),freshObject=hintObject;
    uintptr_t networkObject=reinterpret_cast<uintptr_t>(networkTable.data());
    std::array<uint8_t,0x20> edict{}; std::array<uint8_t,0x60> globals{},handles{};
    int worldObject=0;
    actor=reinterpret_cast<uintptr_t>(bytes.data()); hint=reinterpret_cast<uintptr_t>(&hintObject);
    fresh=reinterpret_cast<uintptr_t>(&freshObject); world=reinterpret_cast<uintptr_t>(&worldObject); network=reinterpret_cast<uintptr_t>(&networkObject);
    entityTable[2]=reinterpret_cast<uintptr_t>(getHandle); networkTable[5]=reinterpret_cast<uintptr_t>(worldEntity);
    at<int>(actor+0x1ec)=query.team; at<uintptr_t>(actor+0x17fc)=99;
    const auto h=reinterpret_cast<uintptr_t>(handles.data());
    at<uintptr_t>(h+20)=query.cached==3?0:hint; at<uint32_t>(h+24)=query.cached==2?3:2;
    at<uintptr_t>(0x105a01e0)=h;
    for(int team=0;team<4;++team) at<uint32_t>(0x106d6c88+4*team)=initialHandle();
    const auto e=reinterpret_cast<uintptr_t>(edict.data()),g=reinterpret_cast<uintptr_t>(globals.data());
    at<uint32_t>(e)=query.world==2?2:0; at<uintptr_t>(e+0xc)=query.world==3?0:network;
    at<uintptr_t>(g+0x58)=query.world==1?0:e; at<uintptr_t>(0x10635ca0)=g;
    at<uintptr_t>(0x1043c234)=reinterpret_cast<uintptr_t>(warning);
    const auto before=bytes;
    selectedId=id(reinterpret_cast<uintptr_t>(reinterpret_cast<void* (__thiscall*)(void*)>(0x10317d70)(bytes.data())));
    cachedHandle=at<uint32_t>(0x106d6c88+4*query.team); typed=at<uintptr_t>(actor+0x17fc);
    auto expected=before; at<uintptr_t>(reinterpret_cast<uintptr_t>(expected.data())+0x17fc)=typed;
    require(bytes==expected,"spawn selector changed unrelated actor bytes");
    for(int team=0;team<4;++team) if(team!=query.team) require(at<uint32_t>(0x106d6c88+4*team)==initialHandle(),"other team spawn cache changed");
}
#else
struct Host {
    int team() { return query.team; }
    uintptr_t recentSpawn() { return initialHint(); }
    bool chooseTeamSpawn(uintptr_t& value) { return choose(value); }
    void cacheSpawn(uintptr_t value) { cachedHandle=value==hint?0x20001:value?freshHandle:UINT32_MAX; }
    void storeTypedSpawn(uintptr_t value) { typed=cast(value); }
    uintptr_t worldEntity() { return worldLookup(); }
    void warnMissingSpawn(bool team) { warn(team); }
};
void run() {
    hint=1; fresh=2; world=3; cachedHandle=initialHandle(); typed=99;
    Host host; selectedId=id(sourceMapSelectSpawnCandidate(host));
}
#endif

int main(int argc,char** argv) {
    try {
        require(argc==2,"expected installed server or -"); (void)argv;
#ifndef _WIN64
        NativePrivateWorldImageFixture image(argv[1],0x6de6c4); image.seal();
        PrivateX86BoundaryObserverFixture observer{{0x10315f80,0x103eb9c7,0x10002800,0x10002600},boundary}; observer.configure(true);
#endif
        unsigned cases=0;
        for(int team:{0,1,2,3}) for(int cached:{0,1,2,3}) for(int choice:{-1,0,1})
        for(int worldState:{0,1,2,3,4}) for(bool success:{false,true}) {
            query={team,cached,choice,worldState,success};
            unsigned expected=0; uintptr_t expectedTyped=99; uint32_t expectedCache=initialHandle(); std::vector<unsigned> expectedEvents;
            if(team>=2) {
                expectedEvents={1}; expected=choice==1?2:choice==0?0:cached==1?1:0;
                if(success) expectedCache=expected==1?0x20001:expected?freshHandle:UINT32_MAX;
                expectedTyped=expected?7:0; expectedEvents.push_back(2);
            } else { if(worldState==0 || worldState==4) expectedEvents.push_back(3); expected=worldState==0?3:0; }
            const bool missing=expected==0;
            if(missing) { expectedEvents.push_back(4); if(worldState==0 || worldState==4) expectedEvents.push_back(3); expected=worldState==0?3:0; }
            events.clear(); warningTeam=false; run();
            require(selectedId==expected && cachedHandle==expectedCache && typed==expectedTyped && events==expectedEvents &&
                (!missing || warningTeam==(team>=2)),"spawn selection mismatch"); ++cases;
        }
#ifndef _WIN64
        observer.configure(false); image.verify();
#endif
        std::cout<<"{\"cases\":"<<cases<<",\"passed\":true}\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
