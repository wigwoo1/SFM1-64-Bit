#pragma once
#include "material_texture_transform_client_native_fixture.h"
// Execute all original proxy/helper instructions unchanged. Only external
// virtual services and referenced DATA are supplied as controlled inputs.
struct MaterialWeaponInvisClientNativeFixture:MaterialTextureTransformClientNativeFixture {
    using Base=MaterialTextureTransformClientNativeFixture;
    std::array<uint8_t,0x310>entity{};
    std::array<uint8_t,0x1900>owner{},viewer{};
    std::array<uint8_t,0x30>limit{};
    std::array<uintptr_t,1>renderMethods{};
    std::array<uintptr_t,8>unknownMethods{};
    std::array<uintptr_t,254>actorMethods{};
    std::array<uintptr_t,127>engineMethods{};
    uintptr_t renderable=0,unknown=0,engine=0;
    std::vector<uint8_t>handleTable;
    std::array<uint32_t,11>q{};
    std::array<uint32_t,9>services{};
    static MaterialWeaponInvisClientNativeFixture&typed(){return *static_cast<MaterialWeaponInvisClientNativeFixture*>(active);}
    static void* __fastcall getUnknown(void*p,void*){auto&f=typed();if(p!=&f.renderable)ExitProcess(110);++f.services[0];return &f.unknown;}
    static void* __fastcall getEntity(void*p,void*){auto&f=typed();if(p!=&f.unknown)ExitProcess(111);++f.services[1];return f.q[0]&4?f.entity.data():nullptr;}
    static bool __fastcall isPlayer(void*p,void*){auto&f=typed();if(p==f.owner.data()){++f.services[2];return (f.q[0]&16)!=0;}if(p==f.viewer.data()){++f.services[3];return (f.q[0]&64)!=0;}ExitProcess(112);}
    static uint32_t __fastcall selector(void*p,void*){auto&f=typed();if(p==f.owner.data()){++f.services[5];return f.q[4];}if(p==f.viewer.data()){++f.services[4];return f.q[5];}ExitProcess(113);}
    static uint32_t __fastcall observerMode(void*p,void*){auto&f=typed();if(p!=f.viewer.data())ExitProcess(114);++f.services[6];return f.q[6];}
    static void* __fastcall observerTarget(void*p,void*){auto&f=typed();if(p!=f.viewer.data())ExitProcess(115);++f.services[7];return f.q[0]&128?f.owner.data():f.entity.data();}
    static uint32_t __fastcall activeSlot(void*p,void*){auto&f=typed();if(p!=&f.engine)ExitProcess(116);++f.services[8];return f.q[9];}
    explicit MaterialWeaponInvisClientNativeFixture(const char*path):Base(path),handleTable(16*65536+16){
        actorMethods.fill(reinterpret_cast<uintptr_t>(&unexpected));unknownMethods.fill(reinterpret_cast<uintptr_t>(&unexpected));engineMethods.fill(reinterpret_cast<uintptr_t>(&unexpected));renderMethods[0]=reinterpret_cast<uintptr_t>(&getUnknown);unknownMethods[7]=reinterpret_cast<uintptr_t>(&getEntity);
        actorMethods[145]=reinterpret_cast<uintptr_t>(&isPlayer);actorMethods[83]=reinterpret_cast<uintptr_t>(&selector);actorMethods[252]=reinterpret_cast<uintptr_t>(&observerMode);actorMethods[253]=reinterpret_cast<uintptr_t>(&observerTarget);engineMethods[126]=reinterpret_cast<uintptr_t>(&activeSlot);
        renderable=reinterpret_cast<uintptr_t>(renderMethods.data());unknown=reinterpret_cast<uintptr_t>(unknownMethods.data());engine=reinterpret_cast<uintptr_t>(engineMethods.data());const auto table=reinterpret_cast<uintptr_t>(actorMethods.data());std::memcpy(owner.data(),&table,4);std::memcpy(viewer.data(),&table,4);
        word(0x63d340,reinterpret_cast<uintptr_t>(handleTable.data()));word(0x6c28ac,reinterpret_cast<uintptr_t>(&engine));word(0x79d524,reinterpret_cast<uintptr_t>(limit.data()));
    }
    void configure(const std::array<uint32_t,11>&input){
        q=input;if(q[9]>1)throw std::runtime_error("Checked local slot0/1");services={};std::memcpy(entity.data()+0x308,&q[2],4);auto*entry=handleTable.data()+16*(q[2]&65535u)+4;const auto pointer=q[0]&8?reinterpret_cast<uintptr_t>(owner.data()):uintptr_t(0);std::memcpy(entry,&pointer,4);std::memcpy(entry+4,&q[3],4);std::memcpy(owner.data()+0x18d8,&q[7],4);std::memcpy(limit.data()+0x2c,&q[8],4);
        word(0x6b57a0,0);word(0x6b57a4,0);word(0x6b57a0+4*q[9],q[0]&32?reinterpret_cast<uintptr_t>(viewer.data()):uintptr_t(0));
    }
    void*createWeapon(){using Call=void*(__cdecl*)();auto*p=reinterpret_cast<Call>(0x1028c5a0)();if(!p||*static_cast<uintptr_t*>(p)!=0x10576dac)throw std::runtime_error("Actual weapon_invis factory/vtable");return p;}
    void*weaponMaterial(void*p){using Call=void*(__thiscall*)(void*);return reinterpret_cast<Call>(0x101b3950)(p);}
    void evaluateWeapon(void*p){using Call=void(__thiscall*)(void*,void*);reinterpret_cast<Call>(0x10175ee0)(p,q[0]&2?&renderable:nullptr);image.verify();}
};
