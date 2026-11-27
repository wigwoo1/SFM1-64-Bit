// Original x86 research only. Explicit services are adapters; no SFM process.
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

template<class T> T& at(uintptr_t address) { return *reinterpret_cast<T*>(address); }
void require(bool value,const char* message) { if (!value) throw std::runtime_error(message); }
uintptr_t actor=0;
std::array<float,3> view{};
unsigned originWrites=0,viewWrites=0,flagNotices=0,solidWrites=0,effectNotices=0,modeCalls=0,dirtyCalls=0;
int requestedMode=0;
void __fastcall changed(void* self,void*,void* field) {
    const auto owner=reinterpret_cast<uintptr_t>(self), address=reinterpret_cast<uintptr_t>(field);
    require((owner==actor && (address==actor+0x120 || address==actor+0xd9 || address==actor+0xd4 || address==actor+0xd8)) ||
            (owner==actor+0x938 && address==actor+0x974) || (owner==actor+0xba0 && address==actor+0xba4),"network change owner/field");
    ++dirtyCalls;
}
const float* __fastcall viewOffset(void* self,void*) { require(reinterpret_cast<uintptr_t>(self)==actor,"view getter owner"); return view.data(); }
void __fastcall setViewOffset(void* self,void*,const float* value) {
    require(reinterpret_cast<uintptr_t>(self)==actor && value[0]==0.f && value[1]==0.f && value[2]==0.f,"view reset arguments");
    view={value[0],value[1],value[2]}; ++viewWrites;
}
bool __fastcall setMode(void* self,void*,int mode) {
    require(reinterpret_cast<uintptr_t>(self)==actor && mode==requestedMode,"observer mode arguments");
    ++modeCalls; return false; // Caller does not propagate this result.
}
void boundary(unsigned slot,CONTEXT& c) {
    using Observer=PrivateX86BoundaryObserverFixture;
    if (slot==0) {
        require(c.Ecx==actor,"origin setter owner");
        const auto* value=reinterpret_cast<const float*>(at<uint32_t>(c.Esp+4));
        std::memcpy(reinterpret_cast<void*>(actor+0x280),value,12); ++originWrites;
        Observer::returnCdecl(c); c.Esp+=4;
    } else if (slot==1) {
        require(c.Ecx==0x1060eeb8 && at<uint32_t>(c.Esp+4)==actor,"entity flag notice owner");
        require(at<uint32_t>(c.Esp+12)==at<uint32_t>(actor+0x120),"entity flag notice result");
        ++flagNotices; Observer::returnCdecl(c); c.Esp+=12;
    } else if (slot==2) {
        require(c.Ecx==actor+0x164,"solid flag setter owner");
        at<uint16_t>(actor+0x184)=static_cast<uint16_t>(at<uint32_t>(c.Esp+4));
        ++solidWrites; Observer::returnCdecl(c); c.Esp+=4;
    } else {
        require(c.Ecx==actor,"effect notice owner");
        ++effectNotices; Observer::returnCdecl(c);
    }
}

int main(int argc,char** argv) {
    try {
        require(argc==2,"expected installed server path");
        NativePrivateWorldImageFixture image(argv[1],0x6de6c4); image.seal();
        PrivateX86BoundaryObserverFixture observer{{0x100c2b60,0x10110ec0,0x100f13e0,0x100b9240},boundary};
        observer.configure(true);
        unsigned cases=0;
        for (uint32_t physics : {0u,8u,0x1040u,0x1048u}) for (bool ready : {false,true})
        for (int mode : {3,6}) for (float time : {0.f,1.25f}) for (unsigned pose=0;pose<3;++pose) {
            alignas(4) std::array<uint8_t,0x1300> bytes{};
            std::array<uintptr_t,0x640/4> table{};
            const uintptr_t notifyTable[]={reinterpret_cast<uintptr_t>(changed)};
            std::array<uint8_t,0x60> globals{};
            actor=reinterpret_cast<uintptr_t>(bytes.data()); requestedMode=mode;
            table[0x23c/4]=reinterpret_cast<uintptr_t>(viewOffset);
            table[0x240/4]=reinterpret_cast<uintptr_t>(setViewOffset);
            table[0x63c/4]=reinterpret_cast<uintptr_t>(setMode);
            for (unsigned offset : {0x2ccu,0x20cu,0x1fcu,0x204u}) table[offset/4]=reinterpret_cast<uintptr_t>(changed);
            at<uintptr_t>(actor)=reinterpret_cast<uintptr_t>(table.data());
            at<uintptr_t>(actor+0x938)=reinterpret_cast<uintptr_t>(notifyTable);
            at<uintptr_t>(actor+0xba0)=reinterpret_cast<uintptr_t>(notifyTable);
            at<uint32_t>(actor+0x918)=UINT32_MAX; // No active weapon.
            at<uint32_t>(actor+0x1f8)=UINT32_MAX; // No ground entity.
            at<uint32_t>(actor+0xc44)=physics;
            at<uint32_t>(actor+0x120)=ready?0x100:0x102;
            at<uint16_t>(actor+0x184)=ready?4:0x20;
            at<int32_t>(actor+0x974)=ready?8:7;
            at<uint8_t>(actor+0xd9)=ready?0:1;
            at<int32_t>(actor+0xd4)=ready?1:0;
            at<uint8_t>(actor+0xd8)=ready?2:0;
            at<uint8_t>(actor+0xba4)=ready?1:0;
            at<uint32_t>(actor+0xb8)=ready?0x20:0x40;
            const std::array<float,3> origin={float(pose)*16.f,-4.f,float(pose)-1.f};
            view={0.5f,float(pose)*2.f,64.f}; const auto priorView=view;
            std::memcpy(reinterpret_cast<void*>(actor+0x280),origin.data(),12);
            at<float>(reinterpret_cast<uintptr_t>(globals.data())+0xc)=time;
            at<int32_t>(reinterpret_cast<uintptr_t>(globals.data())+0x48)=3; // Skip viewport-panel branch.
            at<uintptr_t>(0x10635ca0)=reinterpret_cast<uintptr_t>(globals.data());
            auto expected=bytes; const auto address=reinterpret_cast<uintptr_t>(expected.data());
            at<uint32_t>(address+0xc44)=physics|8;
            at<uint32_t>(address+0x120)&=~2u;
            at<uint16_t>(address+0x184)|=4;
            at<int32_t>(address+0x974)=8; at<uint8_t>(address+0xd9)=0;
            at<int32_t>(address+0xd4)=1; at<uint8_t>(address+0xd8)=2;
            at<uint8_t>(address+0xba4)=1; at<uint32_t>(address+0xb8)|=0x20;
            at<float>(address+0xc84)=time;
            if (!(physics&8)) for (unsigned i=0;i<3;++i) at<float>(address+0x280+i*4)=origin[i]+priorView[i];
            originWrites=viewWrites=flagNotices=solidWrites=effectNotices=modeCalls=dirtyCalls=0;
            for (unsigned repeat=0;repeat<2;++repeat) {
                const auto entry=at<uintptr_t>(0x1050eaac+0x630);
                const auto result=reinterpret_cast<uint32_t (__thiscall*)(void*,int)>(entry)(bytes.data(),mode);
                require(static_cast<uint8_t>(result)==1 && bytes==expected,"observer entry fields/return");
                require(view==((physics&8)?priorView:std::array<float,3>{0,0,0}),"observer view offset");
            }
            require(originWrites==((physics&8)?0u:1u) && viewWrites==originWrites,"observer rebased more than once");
            require(flagNotices==2 && solidWrites==2 && effectNotices==2 && modeCalls==2,"observer service call counts");
            require(dirtyCalls==(ready?0u:6u),"observer changed-field count");
            ++cases;
        }
        observer.configure(false); image.verify();
        std::cout << "{\"cases\":" << cases << ",\"calls_per_case\":2,\"passed\":true}\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
