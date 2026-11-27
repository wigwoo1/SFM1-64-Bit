#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "source_map_noclip_candidate.h"

struct Query { uint32_t cheats, present, isPlayer, mode, argc; int32_t argument;
    uint32_t flags, solid, edict, initialFlags; };
struct Reply { uint32_t mode, collideMode, flags, solid, entityFlags, detached, notified, modeChanged; };
static_assert(sizeof(Query) == 40 && sizeof(Reply) == 32);

#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"

template<class T> T& field(void* base, size_t offset) {
    return *reinterpret_cast<T*>(static_cast<uint8_t*>(base) + offset);
}

void* commandActor = nullptr;
Reply* observed = nullptr;
bool __fastcall rejectNonPlayer(void*, void*) { return false; }
void __fastcall detach(void* actor, void*, void* parent, int attachment) {
    if (actor != commandActor || parent || attachment != -1)
        throw std::runtime_error("unexpected parent operation");
    ++observed->detached;
}
void __fastcall modeChanged(void* actor, void*) {
    if (actor != commandActor) throw std::runtime_error("unexpected actor notification");
    ++observed->modeChanged;
    auto original = *reinterpret_cast<void (__thiscall**)(void*)>(0x1050eaac + 0x5ac);
    original(actor);
}
void boundary(unsigned slot, CONTEXT& c) {
    using Observer = PrivateX86BoundaryObserverFixture;
    auto stack = reinterpret_cast<const uint32_t*>(c.Esp);
    switch (slot) {
    case 0: Observer::returnCdecl(c, reinterpret_cast<uintptr_t>(commandActor)); break;
    case 1:
        if (stack[1] != reinterpret_cast<uintptr_t>(commandActor))
            throw std::runtime_error("unexpected status recipient");
        ++observed->notified;
        Observer::returnCdecl(c);
        break;
    case 2: // Debug-log player name. No actual console output requested.
        Observer::returnCdecl(c, reinterpret_cast<uintptr_t>("fixture"));
        c.Esp += 4;
        break;
    case 3: Observer::returnCdecl(c, std::atoi(reinterpret_cast<const char*>(stack[1]))); break;
    }
}

Reply run(const Query& q) {
    alignas(4) std::array<uint8_t, 0xbc0> actor{};
    alignas(4) std::array<uint8_t, 64> commandVar{}, cheatValue{}, unstuckValue{}, edict{};
    std::array<uintptr_t, 0x5ac / 4 + 1> vtable{};
    vtable[0x94 / 4] = reinterpret_cast<uintptr_t>(detach);
    // Use the installed CTFPlayer primary vtable's predicate for player cases.
    vtable[0x158 / 4] = q.isPlayer ? *reinterpret_cast<const uintptr_t*>(0x1050eaac + 0x158)
                                  : reinterpret_cast<uintptr_t>(rejectNonPlayer);
    vtable[0x5ac / 4] = reinterpret_cast<uintptr_t>(modeChanged);
    field<uintptr_t*>(actor.data(), 0) = vtable.data();
    if (q.edict) field<void*>(actor.data(), 0x18) = edict.data();
    field<uint8_t>(actor.data(), 0x54) = 1; // Track local network dirtiness.
    field<uint32_t>(actor.data(), 0x11c) = q.initialFlags;
    field<uint8_t>(actor.data(), 0x156) = static_cast<uint8_t>(q.mode);
    field<uint8_t>(actor.data(), 0x157) = 1;
    field<uint16_t>(actor.data(), 0x184) = static_cast<uint16_t>(q.flags);
    field<uint8_t>(actor.data(), 0x186) = static_cast<uint8_t>(q.solid);
    field<void*>(commandVar.data(), 0x1c) = cheatValue.data();
    field<uint32_t>(cheatValue.data(), 0x30) = q.cheats;
    *reinterpret_cast<void**>(0x1060a8a0) = commandVar.data();
    *reinterpret_cast<void**>(0x1060ab0c) = unstuckValue.data();
    *reinterpret_cast<uint8_t*>(0x105c14bc) = 0;
    *reinterpret_cast<uint8_t*>(0x105e2530) = 0;
    alignas(4) std::array<uint8_t, 0x500> command{};
    const auto value = std::to_string(q.argument);
    field<uint32_t>(command.data(), 0) = q.argc;
    field<const char*>(command.data(), 0x40c) = value.c_str();
    commandActor = q.present ? actor.data() : nullptr;
    Reply reply{};
    observed = &reply;
    reinterpret_cast<void (__cdecl*)(const void*)>(0x100ee400)(command.data());
    observed = nullptr;
    reply.mode = field<uint8_t>(actor.data(), 0x156);
    reply.collideMode = field<uint8_t>(actor.data(), 0x157);
    reply.flags = field<uint16_t>(actor.data(), 0x184);
    reply.solid = field<uint8_t>(actor.data(), 0x186);
    reply.entityFlags = field<uint32_t>(actor.data(), 0x11c);
    return reply;
}
#else
Reply run(const Query& q) {
    auto change = sourceMapNoclipChange(q.cheats != 0, q.present && q.isPlayer,
        static_cast<uint8_t>(q.mode), q.argc >= 2 ? std::optional<int>(q.argument) : std::nullopt);
    Reply reply{q.mode, 1, q.flags, q.solid, q.initialFlags, 0, 0, 0};
    if (change == SourceMapNoclipChange::unchanged) return reply;
    const bool enter = change == SourceMapNoclipChange::enter;
    reply.mode = enter ? 8u : 2u;
    reply.collideMode = 0;
    reply.entityFlags = enter ? q.initialFlags | 4u : q.initialFlags & ~4u;
    reply.detached = enter;
    reply.notified = reply.modeChanged = 1;
    return reply;
}
#endif

int main(int argc, char** argv) {
    try {
        if (argc != 4) throw std::runtime_error("usage: noclip queries replies server-or-dash");
        std::ifstream in(argv[1], std::ios::binary);
        std::ofstream out(argv[2], std::ios::binary);
        if (!in || !out) throw std::runtime_error("probe input/output");
#ifndef _WIN64
        NativePrivateWorldImageFixture image(argv[3], 0x6de6c4);
        image.seal();
        PrivateX86BoundaryObserverFixture observer{
            {0x10142020, 0x10261da0, 0x10124cf0, 0x103819e0}, boundary};
        observer.configure(true);
#endif
        Query query{};
        while (in.read(reinterpret_cast<char*>(&query), sizeof(query))) {
            const auto result = run(query);
            out.write(reinterpret_cast<const char*>(&result), sizeof(result));
        }
        if (!in.eof() || in.gcount() || !out) throw std::runtime_error("incomplete probe data");
#ifndef _WIN64
        observer.configure(false);
        image.verify();
#endif
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
