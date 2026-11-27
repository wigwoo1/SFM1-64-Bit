#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "source_map_client_activation_candidate.h"
#include "source_map_welcome_observer_candidate.h"

void require(bool value,const char* message) { if (!value) throw std::runtime_error(message); }
std::vector<unsigned> events;
bool locked=false, fake=false, extra=false;
const char* currentName=nullptr;
std::string announcement;
uintptr_t playerOwner=0;
std::array<std::array<uint8_t,0x3b0>,4> observerPoints{};
std::array<bool,4> pointAccepted{};
unsigned pointCount=0,selectedPoint=0;
bool changeStopFlag=false;
unsigned pointIndex(uintptr_t point) {
    for (unsigned i=0;i<pointCount;++i) if (point==reinterpret_cast<uintptr_t>(observerPoints[i].data())) return i;
    throw std::runtime_error("unexpected observer point");
}
uintptr_t nextPoint(uintptr_t previous) {
    const unsigned index=previous?pointIndex(previous)+1:0;
    events.push_back(20+(index<pointCount?index+1:0));
    return index<pointCount?reinterpret_cast<uintptr_t>(observerPoints[index].data()):0;
}
bool acceptsPoint(uintptr_t point) {
    const auto index=pointIndex(point); events.push_back(30+index+1); return pointAccepted[index];
}
void selectPoint(uintptr_t point) {
    const auto index=pointIndex(point); events.push_back(40+index+1); selectedPoint=index+1;
    if (changeStopFlag) observerPoints[index][0x3ad]^=1;
}
void checkPlayer(void* self) { require(reinterpret_cast<uintptr_t>(self)==playerOwner,"activation player owner"); }
bool lockTables(bool value) { const bool prior=locked; events.push_back(value?4:1); locked=value; return prior; }
void initial() { require(!locked,"initial spawn ran locked"); events.push_back(2); }
void spawn() { require(!locked,"spawn ran locked"); events.push_back(3); }
std::array<char,128> copiedName(unsigned limit) {
    require(limit==128,"activation name capacity"); events.push_back(5);
    std::array<char,128> text{};
    for (unsigned i=0;i<127 && currentName[i];++i) text[i]=currentName[i];
    return text;
}
bool isFake() { events.push_back(6); return fake; }
bool extraFlag() { return extra; }
void announce(const char* value) { events.push_back(7); announcement=value; }

#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"
template<class T> T& at(uintptr_t p) { return *reinterpret_cast<T*>(p); }
bool initialOnly=false;
bool observerSelection=false;
void* __fastcall entity(void*,void*) { return reinterpret_cast<void*>(playerOwner); }
bool __fastcall playerCheck(void* self,void*) { checkPlayer(self); return true; }
uint32_t __fastcall stringTableLock(void*,void*,uint32_t value) { return lockTables(static_cast<uint8_t>(value)!=0); }
void __fastcall initialCall(void* self,void*) { checkPlayer(self); initial(); }
void __fastcall spawnCall(void* self,void*) { checkPlayer(self); spawn(); }
const char* __fastcall getName(void* self,void*) { checkPlayer(self); return currentName; }
bool __fastcall fakeCall(void* self,void*) { checkPlayer(self); return isFake(); }
void boundary(unsigned slot,CONTEXT& c) {
    using Observer=PrivateX86BoundaryObserverFixture;
    if (observerSelection) {
        require(slot==0 && c.Ecx==0x1060eeb8,"observer entity-list boundary");
        const auto classname=reinterpret_cast<const char*>(at<uint32_t>(c.Esp+8));
        require(std::strcmp(classname,"info_observer_point")==0,"observer lookup class");
        const auto point=nextPoint(at<uint32_t>(c.Esp+4));
        Observer::returnCdecl(c,point); c.Esp+=8;
        return;
    }
    if (initialOnly) {
        require(c.Ecx==playerOwner,"initial spawn boundary owner");
        if (slot==0) events.push_back(8); // Base initial-spawn body not executed.
        else if (slot==1) {
            require(at<int32_t>(playerOwner+0x15d4)==1 && at<uintptr_t>(playerOwner+0x17f8)==0x105edfd8,
                "welcome entry state");
            events.push_back(12); // Welcome body not executed.
        } else throw std::runtime_error("unexpected initial spawn boundary");
        Observer::returnCdecl(c);
        return;
    }
    if (slot==0) {
        require(reinterpret_cast<const char*>(at<uint32_t>(c.Esp+8))==currentName,"activation source name");
        auto text=copiedName(at<uint32_t>(c.Esp+12));
        const auto destination=at<uint32_t>(c.Esp+4);
        std::memcpy(reinterpret_cast<void*>(destination),text.data(),text.size());
        Observer::returnCdecl(c);
    } else if (slot==1) {
        require(at<uint32_t>(c.Esp+4)==1 && std::strcmp(reinterpret_cast<const char*>(at<uint32_t>(c.Esp+8)),"#Game_connected")==0,"activation notification kind");
        require(!at<uint32_t>(c.Esp+16) && !at<uint32_t>(c.Esp+20) && !at<uint32_t>(c.Esp+24),"activation notification optional arguments");
        announce(reinterpret_cast<const char*>(at<uint32_t>(c.Esp+12)));
        Observer::returnCdecl(c);
    } else throw std::runtime_error("unexpected activation boundary");
}
void run(bool loadGame) {
    alignas(4) std::array<uint8_t,0x1300> playerBytes{};
    std::array<uintptr_t,0x59c/4> playerTable{};
    std::array<uintptr_t,0xd8/4> engineTable{};
    const uintptr_t entityTable[]={0,0,0,0,0,reinterpret_cast<uintptr_t>(entity)};
    const auto instance=reinterpret_cast<uintptr_t>(entityTable);
    alignas(4) std::array<uint8_t,16> edict{};
    playerOwner=reinterpret_cast<uintptr_t>(playerBytes.data());
    playerTable[0x158/4]=reinterpret_cast<uintptr_t>(playerCheck);
    playerTable[0x56c/4]=reinterpret_cast<uintptr_t>(initialCall);
    playerTable[0x60/4]=reinterpret_cast<uintptr_t>(spawnCall);
    playerTable[0xa0/4]=reinterpret_cast<uintptr_t>(getName);
    playerTable[0x598/4]=reinterpret_cast<uintptr_t>(fakeCall);
    engineTable[0xd4/4]=reinterpret_cast<uintptr_t>(stringTableLock);
    at<uintptr_t>(playerOwner)=reinterpret_cast<uintptr_t>(playerTable.data());
    at<uint8_t>(playerOwner+0x126c)=extra;
    at<uintptr_t>(reinterpret_cast<uintptr_t>(edict.data())+0xc)=reinterpret_cast<uintptr_t>(&instance);
    const auto engine=reinterpret_cast<uintptr_t>(engineTable.data());
    at<uintptr_t>(0x10635ca8)=reinterpret_cast<uintptr_t>(&engine);
    const auto before=playerBytes;
    reinterpret_cast<void (__cdecl*)(void*,bool)>(0x102f19e0)(edict.data(),loadGame);
    require(before==playerBytes,"activation adapter fixture mutated player bytes");
}
void __fastcall sharedInitialization(void* self,void*,void* owner) {
    require(reinterpret_cast<uintptr_t>(self)==playerOwner+0x12e8 && reinterpret_cast<uintptr_t>(owner)==playerOwner,
        "shared initialization owner");
    events.push_back(9);
}
void __fastcall initialVirtual(void* self,void*) { checkPlayer(self); events.push_back(10); }
void __fastcall changedState(void* self,void*,void* field) {
    require(reinterpret_cast<uintptr_t>(self)==playerOwner+0x159c && reinterpret_cast<uintptr_t>(field)==playerOwner+0x15d4,
        "state change notification owner");
    require(at<int32_t>(playerOwner+0x15d4)!=1,"unchanged state notification");
    events.push_back(11);
}
unsigned verifyInitialSpawn(PrivateX86BoundaryObserverFixture& observer) {
    observer.configure(false);
    observer.entries={0x101ba1b0,0x1031b300,0x10002800,0x10002600};
    initialOnly=true; observer.configure(true);
    unsigned cases=0;
    for (int32_t state : {-1,0,1,2,3,99}) for (int32_t marker : {-1,0,99})
    for (uint8_t flag : {uint8_t(0),uint8_t(1),uint8_t(255)}) for (bool warm : {false,true}) {
        alignas(4) std::array<uint8_t,0x1900> bytes{};
        std::array<uintptr_t,0x74c/4> table{};
        const uintptr_t sharedTable[]={0,0,0,reinterpret_cast<uintptr_t>(sharedInitialization)};
        const uintptr_t changedTable[]={reinterpret_cast<uintptr_t>(changedState)};
        std::array<uint8_t,0x34> debugVariable{};
        int disabledFeature=0;
        playerOwner=reinterpret_cast<uintptr_t>(bytes.data());
        table[0x748/4]=reinterpret_cast<uintptr_t>(initialVirtual);
        at<uintptr_t>(playerOwner)=reinterpret_cast<uintptr_t>(table.data());
        at<uintptr_t>(playerOwner+0x12e8)=reinterpret_cast<uintptr_t>(sharedTable);
        at<uintptr_t>(playerOwner+0x159c)=reinterpret_cast<uintptr_t>(changedTable);
        at<int32_t>(playerOwner+0x15d4)=state;
        at<int32_t>(playerOwner+0x1838)=marker;
        at<uint8_t>(playerOwner+0x1591)=flag;
        at<uintptr_t>(0x105b1a6c)=reinterpret_cast<uintptr_t>(&disabledFeature);
        at<int32_t>(reinterpret_cast<uintptr_t>(debugVariable.data())+0x30)=2; // Suppress transition log for index 0.
        at<uintptr_t>(0x106d6cb4)=reinterpret_cast<uintptr_t>(debugVariable.data());
        if (!warm) at<uint32_t>(0x106d6c80)=0;
        require(!warm || at<uint32_t>(0x106d6c80)==1,"warm state table not initialized");
        auto expected=bytes;
        const auto expectedOwner=reinterpret_cast<uintptr_t>(expected.data());
        at<int32_t>(expectedOwner+0x15d4)=1;
        at<uintptr_t>(expectedOwner+0x17f8)=0x105edfd8;
        at<int32_t>(expectedOwner+0x1838)=-1;
        at<uint8_t>(expectedOwner+0x1591)=0;
        events.clear();
        const auto initialAddress=at<uintptr_t>(0x1050eaac+0x56c);
        reinterpret_cast<void (__thiscall*)(void*)>(initialAddress)(bytes.data());
        std::vector<unsigned> expectedEvents={8,9,10};
        if (state!=1) expectedEvents.push_back(11);
        expectedEvents.push_back(12);
        require(events==expectedEvents && bytes==expected,"initial spawn state or sequence");
        require(std::strcmp(reinterpret_cast<const char*>(at<uintptr_t>(0x105edfd8+4)),"TF_STATE_WELCOME")==0 &&
            at<uintptr_t>(0x105edfd8+8)==0x1031b300,"installed welcome record");
        ++cases;
    }
    initialOnly=false; return cases;
}
bool __fastcall acceptsPointCall(void* self,void*,void* point) {
    checkPlayer(self); return acceptsPoint(reinterpret_cast<uintptr_t>(point));
}
void __fastcall selectPointCall(void* self,void*,void* point) {
    checkPlayer(self); selectPoint(reinterpret_cast<uintptr_t>(point));
}
void runObserverSearch() {
    std::array<uintptr_t,0x660/4> table{};
    table[0x65c/4]=reinterpret_cast<uintptr_t>(acceptsPointCall);
    table[0x644/4]=reinterpret_cast<uintptr_t>(selectPointCall);
    const auto owner=reinterpret_cast<uintptr_t>(table.data());
    playerOwner=reinterpret_cast<uintptr_t>(&owner);
    reinterpret_cast<void (__thiscall*)(void*)>(0x10314f50)(reinterpret_cast<void*>(playerOwner));
}
#else
struct Host {
    bool lockStringTables(bool value) { return lockTables(value); }
    void initialSpawn() { initial(); }
    void spawn() { ::spawn(); }
    auto announcementName(unsigned limit) { return copiedName(limit); }
    bool fakeClient() { return isFake(); }
    bool extraClientFlag() { return extraFlag(); }
    void announce(const char* value) { ::announce(value); }
};
void run(bool loadGame) { (void)loadGame; Host host; sourceMapFinishClientActivation(host); }
struct WelcomeHost {
    uintptr_t next(uintptr_t previous) { return nextPoint(previous); }
    bool accepts(uintptr_t point) { return acceptsPoint(point); }
    void select(uintptr_t point) { selectPoint(point); }
    bool stopAfter(uintptr_t point) { return observerPoints[pointIndex(point)][0x3ad]!=0; }
};
void runObserverSearch() { WelcomeHost host; sourceMapSelectWelcomeObserver(host); }
#endif

unsigned verifyObserverSearch() {
    unsigned cases=0;
    for (unsigned count=0;count<=4;++count)
    for (unsigned valid=0;valid<(1u<<count);++valid)
    for (unsigned stop=0;stop<(1u<<count);++stop)
    for (bool mutate : {false,true}) for (unsigned prior : {0u,77u}) {
        pointCount=count; changeStopFlag=mutate; selectedPoint=prior;
        for (unsigned i=0;i<4;++i) {
            observerPoints[i].fill(0xa5);
            pointAccepted[i]=(valid&(1u<<i))!=0;
            observerPoints[i][0x3ad]=(stop&(1u<<i))?1:0;
        }
        auto expectedPoints=observerPoints;
        std::vector<unsigned> expectedEvents;
        unsigned expectedSelection=prior;
        for (unsigned i=0;;++i) {
            if (i==count) { expectedEvents.push_back(20); break; }
            expectedEvents.push_back(20+i+1); expectedEvents.push_back(30+i+1);
            if (valid&(1u<<i)) {
                expectedSelection=i+1; expectedEvents.push_back(40+i+1);
                if (mutate) expectedPoints[i][0x3ad]^=1;
            }
            if (expectedPoints[i][0x3ad]) break;
        }
        events.clear(); runObserverSearch();
        require(events==expectedEvents && selectedPoint==expectedSelection && observerPoints==expectedPoints,
            "welcome observer search sequence/state");
        ++cases;
    }
    return cases;
}

int main(int argc,char** argv) {
    try {
        require(argc==2,"expected installed server or -"); (void)argv;
#ifndef _WIN64
        NativePrivateWorldImageFixture image(argv[1],0x6de6c4); image.seal();
        PrivateX86BoundaryObserverFixture observer{{0x10381bc0,0x10261d40,0x10002800,0x10002600},boundary};
        observer.configure(true);
#endif
        const std::array<std::string,6> names={"", "SFM64", "%A%%", std::string(127,'%'),std::string(128,'B'),std::string(200,'C')};
        unsigned cases=0;
        for (bool load : {false,true}) for (bool prior : {false,true})
        for (bool bot : {false,true}) for (bool identity : {false,true}) for (const auto& text : names) {
            locked=prior; fake=bot; extra=identity; currentName=text.c_str(); events.clear(); announcement.clear();
            run(load);
            std::vector<unsigned> expected={1,2,3,prior?4u:1u,5,6};
            std::string expectedMessage;
            if (!bot || identity) {
                expected.push_back(7); expectedMessage=text.substr(0,127);
                for (auto& character : expectedMessage) if (character=='%') character=' ';
                if (expectedMessage.empty()) expectedMessage="<unconnected>";
            }
            require(events==expected && announcement==expectedMessage && locked==prior,"activation event sequence or result");
            ++cases;
        }
        unsigned initialCases=0;
#ifndef _WIN64
        initialCases=verifyInitialSpawn(observer);
        observer.configure(false);
        observer.entries={0x1010f010,0x10002800,0x10002600,0x10002700};
        observerSelection=true; observer.configure(true);
#endif
        const auto observerCases=verifyObserverSearch();
#ifndef _WIN64
        observer.configure(false); image.verify();
#endif
        std::cout << "{\"cases\":" << cases << ",\"initial_cases\":" << initialCases
                  << ",\"observer_cases\":" << observerCases << ",\"passed\":true}\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
