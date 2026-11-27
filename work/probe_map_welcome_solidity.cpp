// Original x86 composition only; no live engine or production x64 actor.
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
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"

void require(bool v,const char* s) { if(!v) throw std::runtime_error(s); }
template<class T> T& at(uintptr_t p) { return *reinterpret_cast<T*>(p); }
uintptr_t actor=0,rules=0,engine=0;
unsigned modeCalls=0,movementCalls=0,searchCalls=0,triggerCalls=0,panelCalls=0;
bool ruleResult=false;
bool suffixPhase=false;
unsigned fovCalls=0,viewCalls=0,endCalls=0;
const std::array<float,9> ruleBounds={0,0,64,-24,-24,0,24,24,82};
// Reproduce the stack/register contract of the ORIGINAL common Spawn block.
// The prefix and active-state branch are not executed or claimed verified.
__declspec(naked) void __cdecl runSpawnSuffix(void*,void*) {
    __asm {
        push ebp
        mov ebp,esp
        sub esp,4ch
        push ebx
        push esi
        push edi
        xor ebx,ebx
        mov esi,dword ptr [ebp+8]
        mov eax,dword ptr [ebp+12]
        mov dword ptr [ebp-14h],eax
        mov eax,1032269bh
        jmp eax
    }
}
const float* __fastcall bounds(void* self,void*) { require(reinterpret_cast<uintptr_t>(self)==rules,"bounds rules owner"); return ruleBounds.data(); }
void __fastcall endSpawn(void* self,void*) { require(reinterpret_cast<uintptr_t>(self)==engine,"spawn suffix end owner"); ++endCalls; }
void __fastcall changed(void* self,void*,void* field) {
    const auto owner=reinterpret_cast<uintptr_t>(self), address=reinterpret_cast<uintptr_t>(field);
    require(owner>=actor && owner<actor+0x1900 && address>=actor && address<actor+0x1900,"changed field outside actor");
}
bool __fastcall setMode(void* self,void*,int mode) {
    require(reinterpret_cast<uintptr_t>(self)==actor && mode==3,"welcome mode request"); ++modeCalls; return false;
}
bool __fastcall ruleQuery(void* self,void*,void* player) {
    require(reinterpret_cast<uintptr_t>(self)==rules && reinterpret_cast<uintptr_t>(player)==actor,"welcome rule query"); return ruleResult;
}
void __fastcall panel(void* self,void*,int value) {
    require(reinterpret_cast<uintptr_t>(self)==actor && value==1 && at<uint8_t>(actor+0x17d6)==1,"panel state request"); ++panelCalls;
}
int __fastcall collisionMode(void* self,void*) { require(reinterpret_cast<uintptr_t>(self)==actor+0x164,"collision owner"); return 2; }
void __fastcall triggerUpdate(void* self,void*,void* edict,int flag) {
    require(reinterpret_cast<uintptr_t>(self)==engine && reinterpret_cast<uintptr_t>(edict)==at<uintptr_t>(actor+0x18) && flag==0,"trigger arguments"); ++triggerCalls;
}
void boundary(unsigned slot,CONTEXT& c) {
    using O=PrivateX86BoundaryObserverFixture;
    if(suffixPhase) {
        if(slot==1) { require(c.Ecx==actor,"suffix view owner"); ++viewCalls; c.EFlags|=0x10000; return; }
        require(slot==0 && c.Ecx==actor && at<uintptr_t>(c.Esp+4)==actor && at<int>(c.Esp+8)==0 &&
            at<int>(c.Esp+12)==0 && at<int>(c.Esp+16)==0,"suffix FOV arguments");
        ++fovCalls; O::returnCdecl(c); c.Esp+=16; return;
    }
    if(slot==0) {
        require(c.Ecx==actor && at<int>(c.Esp+4)==0 && at<int>(c.Esp+8)==0,"welcome movement request"); ++movementCalls;
        O::returnCdecl(c); c.Esp+=8;
    } else if(slot==1) { require(c.Ecx==actor,"effect notice owner"); O::returnCdecl(c); }
    else if(slot==2) {
        require(c.Ecx==0x1060eeb8 && at<uintptr_t>(c.Esp+4)==0 && std::strcmp(reinterpret_cast<const char*>(at<uintptr_t>(c.Esp+8)),"info_observer_point")==0,"observer search");
        ++searchCalls; O::returnCdecl(c); c.Esp+=8;
    } else {
        require(c.Ecx==0x1060eeb8 && at<uintptr_t>(c.Esp+4)==actor,"entity flag notice"); O::returnCdecl(c); c.Esp+=12;
    }
}

int main(int argc,char** argv) {
    try {
        require(argc==2,"expected installed server DLL");
        NativePrivateWorldImageFixture image(argv[1],0x6de6c4); image.seal();
        PrivateX86BoundaryObserverFixture observer{{0x100c2710,0x100b9240,0x1010f010,0x10110ec0},boundary}; observer.configure(true);
        unsigned cases=0;
        for(uint16_t flags:{uint16_t(0),uint16_t(4),uint16_t(8),uint16_t(12)}) for(bool ready:{false,true})
        for(float time:{0.f,1.25f}) for(bool special:{false,true}) {
            alignas(4) std::array<uint8_t,0x1900> bytes{};
            std::array<uintptr_t,0x640/4> table{};
            std::array<uintptr_t,0x4c/4> collisionTable{};
            std::array<uintptr_t,0x134/4> engineTable{};
            const uintptr_t notifyTable[]={reinterpret_cast<uintptr_t>(changed)};
            std::array<uintptr_t,0x7c/4> rulesTable{};
            rulesTable[3]=reinterpret_cast<uintptr_t>(ruleQuery); rulesTable[0x78/4]=reinterpret_cast<uintptr_t>(bounds);
            uintptr_t ruleObject=reinterpret_cast<uintptr_t>(rulesTable.data()),engineObject=reinterpret_cast<uintptr_t>(engineTable.data());
            std::array<uint8_t,0x60> globals{};
            actor=reinterpret_cast<uintptr_t>(bytes.data()); rules=reinterpret_cast<uintptr_t>(&ruleObject); engine=reinterpret_cast<uintptr_t>(&engineObject);
            table[0x630/4]=0x101c5090; table[0x63c/4]=reinterpret_cast<uintptr_t>(setMode); table[0x17c/4]=reinterpret_cast<uintptr_t>(panel);
            table[0x240/4]=0x100ce660; table[0x30c/4]=reinterpret_cast<uintptr_t>(changed); engineTable[0x78/4]=reinterpret_cast<uintptr_t>(endSpawn);
            for(unsigned slot:{0x2ccu,0x20cu,0x1fcu,0x204u}) table[slot/4]=reinterpret_cast<uintptr_t>(changed);
            collisionTable[0x48/4]=reinterpret_cast<uintptr_t>(changed); collisionTable[0x2c/4]=reinterpret_cast<uintptr_t>(collisionMode);
            engineTable[0x130/4]=reinterpret_cast<uintptr_t>(triggerUpdate);
            at<uintptr_t>(actor)=reinterpret_cast<uintptr_t>(table.data());
            at<uintptr_t>(actor+0x164)=reinterpret_cast<uintptr_t>(collisionTable.data()); at<uintptr_t>(actor+0x168)=actor;
            at<uint16_t>(actor+0x18c)=0xffff; at<uint16_t>(actor+0x184)=flags;
            at<uintptr_t>(actor+0x938)=reinterpret_cast<uintptr_t>(notifyTable); at<uintptr_t>(actor+0xba0)=reinterpret_cast<uintptr_t>(notifyTable);
            at<uint32_t>(actor+0x918)=UINT32_MAX; at<uint32_t>(actor+0x1f8)=UINT32_MAX;
            at<uint32_t>(actor+0xca8)=UINT32_MAX; at<uint32_t>(actor+0x189c)=UINT32_MAX;
            at<uintptr_t>(actor+0x159c)=reinterpret_cast<uintptr_t>(notifyTable);
            at<int>(actor+0x1810)=ready?10:1;
            at<uint32_t>(actor+0xc44)=8; at<uint32_t>(actor+0x11c)=0x1000000;
            at<uint32_t>(actor+0x120)=ready?0x100:0x102; at<int>(actor+0x974)=ready?8:7;
            at<uint8_t>(actor+0xd9)=ready?0:1; at<int>(actor+0xd4)=ready?1:0; at<uint8_t>(actor+0xd8)=ready?2:0;
            at<uint8_t>(actor+0xba4)=ready?1:0; at<uint32_t>(actor+0xb8)=ready?0x30:0x40;
            at<uint8_t>(actor+0x1868)=1;
            const std::array<float,3> initialOrigin=ready?std::array<float,3>{128.f,-300.f,64.f}:std::array<float,3>{-45.f,70.f,-6.f};
            std::memcpy(bytes.data()+0x280,initialOrigin.data(),12);
            const auto g=reinterpret_cast<uintptr_t>(globals.data()); at<float>(g+0xc)=time; at<int>(g+0x48)=3;
            at<uintptr_t>(0x10635ca0)=g; at<uintptr_t>(0x105d5ec0)=rules; ruleResult=special;
            auto expected=bytes; const auto e=reinterpret_cast<uintptr_t>(expected.data());
            at<uint32_t>(e+0x120)&=~2u; at<uint16_t>(e+0x184)|=4;
            at<int>(e+0x974)=8; at<uint8_t>(e+0xd9)=0; at<int>(e+0xd4)=1; at<uint8_t>(e+0xd8)=2;
            at<uint8_t>(e+0xba4)=1; at<uint32_t>(e+0xb8)|=0x30; at<float>(e+0xc84)=time;
            at<uint8_t>(e+0x17d6)=1; at<uint8_t>(e+0x1868)=0;
            modeCalls=movementCalls=searchCalls=triggerCalls=panelCalls=0;
            for(int repeat=0;repeat<2;++repeat) reinterpret_cast<void (__thiscall*)(void*)>(0x1031b300)(bytes.data());
            require(bytes==expected && modeCalls==2 && movementCalls==2 && searchCalls==2 && panelCalls==2,"welcome/controller composition");
            observer.configure(false); suffixPhase=true;
            observer.entries={0x100ddbe0,0x100ce660,0x10002600,0x10002400}; observer.configure(true);
            at<uintptr_t>(0x10636664)=rules;
            at<uintptr_t>(0x106cdfc0+0x74)=0; // No scoreboard storage.
            const auto priorFlags=at<uint16_t>(actor+0x184);
            std::array<uint8_t,12> priorOrigin{}; std::memcpy(priorOrigin.data(),bytes.data()+0x280,12);
            fovCalls=viewCalls=endCalls=0;
            for(unsigned repeat=0;repeat<2;++repeat) {
                runSpawnSuffix(bytes.data(),reinterpret_cast<void*>(engine));
                require(at<uint16_t>(actor+0x184)==priorFlags && std::memcmp(priorOrigin.data(),bytes.data()+0x280,12)==0,"spawn suffix changed solidity/origin");
                require(at<uint8_t>(actor+0x1780)==uint8_t(repeat==0),"spawn toggle field");
                require(at<float>(actor+0x18a0)==time && at<float>(actor+0x12b8)==time && at<float>(actor+0x12c0)==time,"spawn suffix time fields");
                require(at<uint8_t>(actor+0x18e)==3 && at<int>(actor+0x18d8)==-1,"spawn suffix bounds mode/reset");
                require(std::memcmp(bytes.data()+0x190,ruleBounds.data()+3,12)==0 &&
                    std::memcmp(bytes.data()+0x19c,ruleBounds.data()+6,12)==0,"spawn suffix bounds");
                require(std::memcmp(bytes.data()+0x358,reinterpret_cast<const void*>(0x105ec728+12*(ready?10:1)),12)==0,"spawn suffix class view offset");
            }
            require(fovCalls==2 && viewCalls==2 && endCalls==2,"spawn suffix service counts");
            observer.configure(false); suffixPhase=false;
            observer.entries={0x100c2710,0x100b9240,0x1010f010,0x10110ec0}; observer.configure(true);
            // Give the post-welcome actor a disposable edict only for admission.
            int dummyEdict=0; at<uintptr_t>(actor+0x18)=reinterpret_cast<uintptr_t>(&dummyEdict); at<uint8_t>(actor+0x186)=2;
            at<uintptr_t>(0x10635ca8)=engine; at<uintptr_t>(0x106716d4)=0; at<uint8_t>(0x105938fc)=0;
            reinterpret_cast<void (__thiscall*)(void*,void*)>(0x100cddb0)(bytes.data(),nullptr);
            require(triggerCalls==unsigned(flags&8?1:0),"welcome solidity trigger admission"); ++cases;
        }
        observer.configure(false); image.verify();
        std::cout<<"{\"cases\":"<<cases<<",\"welcome_calls_per_case\":2,\"suffix_calls_per_case\":2,\"passed\":true}\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
