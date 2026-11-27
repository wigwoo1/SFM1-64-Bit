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
#include "source_map_client_creation_candidate.h"

void require(bool v, const char* message) { if (!v) throw std::runtime_error(message); }
std::array<uint8_t, 0x1000> player{};
const char* name = nullptr;
void* edict = nullptr;
unsigned creates = 0, overrides = 0, copies = 0, lookups = 0;
bool useOverride = false;

void allocate() { ++creates; }
void copyName(void* destination, const char* source, unsigned limit) {
    require(destination == player.data()+0xfbd && source == name && limit == 32, "name copy arguments");
    auto* output = static_cast<char*>(destination);
    bool ended = false;
    for (unsigned i = 0; i < limit; ++i) {
        if (!ended && source[i] == 0) ended = true;
        output[i] = ended ? 0 : source[i];
    }
    ++copies;
}

#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"
template<class T> T& cell(uintptr_t p) { return *reinterpret_cast<T*>(p); }
void __cdecl overrideCreate(void* suppliedEdict, const char* suppliedName) {
    require(suppliedEdict == edict && suppliedName == name, "creation override arguments");
    ++overrides;
}
void* __fastcall resolveEntity(void*, void*) { ++lookups; return nullptr; }
void boundary(unsigned slot, CONTEXT& c) {
    using Observer = PrivateX86BoundaryObserverFixture;
    if (slot == 0) {
        const char* classname = reinterpret_cast<const char*>(cell<uint32_t>(c.Esp+4));
        require(std::strcmp(classname,"player") == 0 && cell<int32_t>(c.Esp+8) == -1 &&
                cell<uint32_t>(c.Esp+12) == 1, "entity creation arguments");
        require(cell<void*>(0x1063dbe0) == edict, "edict published before allocation");
        allocate();
        Observer::returnCdecl(c,reinterpret_cast<uintptr_t>(player.data()));
    } else if (slot == 1) {
        auto* destination = reinterpret_cast<void*>(cell<uint32_t>(c.Esp+4));
        copyName(destination,reinterpret_cast<const char*>(cell<uint32_t>(c.Esp+8)),cell<uint32_t>(c.Esp+12));
        Observer::returnCdecl(c,reinterpret_cast<uintptr_t>(destination));
    } else throw std::runtime_error("unexpected client creation boundary");
}
void run() {
    std::array<uint8_t, 16> slot{};
    const uintptr_t table[] = {0,0,0,0,0,reinterpret_cast<uintptr_t>(resolveEntity)};
    const auto entity = reinterpret_cast<uintptr_t>(table);
    if (edict) {
        edict = slot.data();
        cell<uintptr_t>(reinterpret_cast<uintptr_t>(slot.data())+0xc) = reinterpret_cast<uintptr_t>(&entity);
    }
    cell<uintptr_t>(0x10635d34) = useOverride ? reinterpret_cast<uintptr_t>(overrideCreate) : 0;
    cell<uintptr_t>(0x1063dbe0) = 0x1234;
    const auto tableAddress = cell<uintptr_t>(0x105b1ca0);
    reinterpret_cast<void (__thiscall*)(void*, void*, const char*)>(cell<uintptr_t>(tableAddress+5*4))(
        reinterpret_cast<void*>(0x105b1ca0),edict,name);
    require(cell<uintptr_t>(0x1063dbe0) == (useOverride ? 0x1234 : reinterpret_cast<uintptr_t>(edict)), "creation edict handoff");
    require(lookups == (edict ? 1u : 0u), "edict entity lookup count");
}
#else
struct Host {
    bool hasOverride() { return useOverride; }
    void overrideCreate() { ++overrides; }
    void* create(const char* classname, int forcedIndex, bool precache) {
        require(std::strcmp(classname,"player") == 0 && forcedIndex == -1 && precache, "owned creation arguments");
        allocate(); return player.data();
    }
    bool hasName() { return name != nullptr; }
    void setName(void* p, unsigned limit) {
        require(p == player.data(), "owned player owner");
        copyName(static_cast<uint8_t*>(p)+0xfbd,name,limit);
        player[0xfbd+limit-1] = 0;
    }
};
void run() { Host host; sourceMapCreateClientPlayer(host); }
#endif

int main(int argc, char** argv) {
    try {
        require(argc == 2,"expected installed server path or -");
        (void)argv;
#ifndef _WIN64
        NativePrivateWorldImageFixture image(argv[1],0x6de6c4);
        image.seal();
        PrivateX86BoundaryObserverFixture observer{{0x1016fe60,0x103f1da0,0x10002800,0x10002600},boundary};
        observer.configure(true);
#endif
        const std::array<std::string,5> names = {"", "SFM64", std::string(31,'A'), std::string(32,'B'), std::string(80,'C')};
        unsigned cases = 0;
        for (bool hook : {false,true}) for (bool hasEdict : {false,true})
        for (int index = -1; index < static_cast<int>(names.size()); ++index) {
            player.fill(0xa5); auto expected = player;
            useOverride = hook; edict = hasEdict ? reinterpret_cast<void*>(1) : nullptr;
            name = index < 0 ? nullptr : names[index].c_str();
            creates = overrides = copies = lookups = 0;
            run();
            require(creates == (!hook ? 1u : 0u) && overrides == (hook ? 1u : 0u) &&
                    copies == (!hook && name ? 1u : 0u), "creation route counts");
            if (!hook && name) {
                const auto length = names[index].size();
                for (unsigned j = 0; j < 32; ++j) expected[0xfbd+j] = j < length && j < 31 ? static_cast<uint8_t>(name[j]) : 0;
            }
            require(player == expected,"name bytes or unrelated player bytes changed");
            ++cases;
        }
#ifndef _WIN64
        observer.configure(false); image.verify();
#endif
        std::cout << "{\"cases\":" << cases << ",\"passed\":true}\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
