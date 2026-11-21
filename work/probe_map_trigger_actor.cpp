#include "source_map_trigger_actor_candidate.h"

#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>

struct Query {
    uint32_t clientPresent, handle, serverSerial, directSlot;
    int32_t maxClients;
    uint32_t localSlot, directMove, localMove;
    uint32_t actorFlags, solidType, solidFlags, hasEdict, isWorld, modelState;
    uint32_t priorOrigin, accurate, moveType, alreadyQueued;
};

struct Reply {
    uint32_t mapped, local, reportsNoclip, passesClientFilter;
    uint32_t scan, checkUntouch, addedToQueue, priorOrigin, accurate;
};
static_assert(sizeof(Query) == 72 && sizeof(Reply) == 36);

#ifdef NATIVE_MAP_TRIGGER_ACTOR
#include <windows.h>
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"

template<typename T>
T& cell(void* base, size_t offset) {
    return *reinterpret_cast<T*>(static_cast<uint8_t*>(base) + offset);
}

template<typename T>
T& global(uintptr_t address) {
    return *reinterpret_cast<T*>(address);
}

struct Interface {
    const uintptr_t* table;
    void* value;
};

void* __fastcall interfaceValue(Interface* object, void*) {
    return object->value;
}

uint32_t __fastcall getSolidType(void* collision, void*) {
    return cell<uint8_t>(collision, 0x22);
}

struct NativeTouchObservation {
    Reply& reply;
    void* actor;
    void* edict;
    void* previousOrigin;
    bool accurate;
};
static NativeTouchObservation* observed;

void __cdecl registerUntouch(void* actor) {
    if (actor != observed->actor) throw std::runtime_error("check-untouch actor mismatch");
    ++observed->reply.addedToQueue;
}

void boundary(unsigned slot, CONTEXT& context) {
    if (slot != 0) throw std::runtime_error("unexpected native boundary");
    auto* words = reinterpret_cast<const uintptr_t*>(context.Esp);
    registerUntouch(reinterpret_cast<void*>(words[1]));
    PrivateX86BoundaryObserverFixture::returnCdecl(context);
}

void checkNotification(void* edict, uint32_t accurate, SourceMapTouchScanCandidate scan) {
    auto& reply = observed->reply;
    if (edict != observed->edict || accurate != static_cast<uint32_t>(observed->accurate) || reply.scan ||
        !(cell<uint32_t>(observed->actor, 0x11c) & 0x1000000)) {
        throw std::runtime_error("unexpected touch scan callback");
    }
    reply.scan = static_cast<uint32_t>(scan);
    reply.accurate = accurate;
}

void __fastcall solidMoved(void*, void*, void* edict, void* collision, void* previous, uint32_t accurate) {
    if (collision != static_cast<uint8_t*>(observed->actor) + 0x164 || previous != observed->previousOrigin) {
        throw std::runtime_error("unexpected solid collision data");
    }
    checkNotification(edict, accurate, SourceMapTouchScanCandidate::solidMoved);
    observed->reply.priorOrigin = previous != nullptr;
}

void __fastcall triggerMoved(void*, void*, void* edict, uint32_t accurate) {
    checkNotification(edict, accurate, SourceMapTouchScanCandidate::triggerMoved);
}

uintptr_t __fastcall modelIndex(void*, void*) { return 1; }
void* __fastcall lookupModel(Interface* object, void*, uintptr_t index) {
    if (index != 1) throw std::runtime_error("unexpected fixture model index");
    return object->value;
}

const char** __fastcall getModelName(void* object, void*, const char** name) {
    *name = cell<const char*>(object, 0x20);
    return name;
}

uint8_t __fastcall noParent(void*, void*) { return 0; }
void __cdecl ignoreWarning(const char*, ...) {}

Reply run(const Query& query) {
    Reply reply{};
    alignas(4) std::array<uint8_t, 0x180> direct{}, local{};
    cell<uint8_t>(direct.data(), 0x156) = static_cast<uint8_t>(query.directMove);
    cell<uint8_t>(local.data(), 0x156) = static_cast<uint8_t>(query.localMove);
    uintptr_t networkableTable[6]{};
    networkableTable[5] = reinterpret_cast<uintptr_t>(interfaceValue);
    Interface directNetworkable{networkableTable, query.directSlot == 2 ? direct.data() : nullptr};
    Interface localNetworkable{networkableTable, query.localSlot == 4 ? local.data() : nullptr};
    uintptr_t clientTable[3]{};
    clientTable[2] = reinterpret_cast<uintptr_t>(interfaceValue);
    uint32_t handle = query.handle;
    Interface client{clientTable, &handle};
    void* source = query.clientPresent ? &client : nullptr;
    uint32_t tools = 0x10525284;

    const uint32_t index = handle & 0xffff;
    if (index < 2048) {
        global<Interface*>(0x1060eebc + index * 16) = query.directSlot ? &directNetworkable : nullptr;
        global<uint32_t>(0x1060eec0 + index * 16) = query.serverSerial;
    }
    std::array<uint32_t, 8> edicts{};
    edicts[4] = query.localSlot == 1 ? 2u : 0u;
    if (query.localSlot == 1 || query.localSlot >= 3) {
        edicts[7] = reinterpret_cast<uint32_t>(&localNetworkable);
    }
    std::array<uint32_t, 24> globals{};
    globals[0x14 / 4] = static_cast<uint32_t>(query.maxClients);
    globals[0x58 / 4] = query.localSlot ? reinterpret_cast<uint32_t>(edicts.data()) : 0u;
    global<uint32_t*>(0x10635ca0) = globals.data();

    using Mapping = void* (__thiscall*)(void*, void*);
    using NoClip = bool (__thiscall*)(void*, void*);
    reply.mapped = reinterpret_cast<Mapping>(0x10343c60)(&tools, source) == direct.data();
    reply.local = reinterpret_cast<void* (__cdecl*)()>(0x102637e0)() == local.data();
    reply.reportsNoclip = reinterpret_cast<NoClip>(0x10343d50)(&tools, source);
    if (index < 2048) global<Interface*>(0x1060eebc + index * 16) = nullptr;

    alignas(4) std::array<uint8_t, 0x190> actor{};
    uintptr_t actorTable[0x158 / 4 + 1]{};
    actorTable[0x18 / 4] = reinterpret_cast<uintptr_t>(modelIndex);
    actorTable[0x1c / 4] = reinterpret_cast<uintptr_t>(getModelName);
    actorTable[0x158 / 4] = reinterpret_cast<uintptr_t>(noParent);
    cell<const uintptr_t*>(actor.data(), 0) = actorTable;
    cell<const char*>(actor.data(), 0x20) = query.modelState == 1 ? "*2" : nullptr;
    cell<uint32_t>(actor.data(), 0x120) = query.actorFlags;
    cell<uint8_t>(actor.data(), 0x156) = static_cast<uint8_t>(query.moveType);
    alignas(4) std::array<uint8_t, 0x430> trigger{};
    cell<uint32_t>(trigger.data(), 0x118) = 1;
    cell<uint32_t>(trigger.data(), 0x428) = UINT32_MAX;
    reply.passesClientFilter = reinterpret_cast<bool (__thiscall*)(void*, void*)>(0x102563e0)(
        trigger.data(), actor.data());

    uintptr_t collisionTable[12]{};
    collisionTable[0x2c / 4] = reinterpret_cast<uintptr_t>(getSolidType);
    cell<const uintptr_t*>(actor.data(), 0x164) = collisionTable;
    cell<uint16_t>(actor.data(), 0x184) = static_cast<uint16_t>(query.solidFlags);
    cell<uint8_t>(actor.data(), 0x186) = static_cast<uint8_t>(query.solidType);
    cell<uint32_t>(actor.data(), 0x11c) = query.alreadyQueued ? 0x1000000u : 0u;
    cell<void*>(actor.data(), 0x18) = query.hasEdict ? edicts.data() : nullptr;

    uintptr_t modelTable[2]{};
    modelTable[1] = reinterpret_cast<uintptr_t>(lookupModel);
    Interface modelInfo{modelTable, query.modelState == 2 ? actor.data() : nullptr};
    global<Interface*>(0x10635cc8) = &modelInfo;
    global<decltype(&ignoreWarning)>(0x1043c234) = ignoreWarning;
    global<void*>(0x106716d4) = query.isWorld ? actor.data() : nullptr;
    global<uint8_t>(0x105938fc) = static_cast<uint8_t>(query.accurate);

    uintptr_t engineTable[0x130 / 4 + 1]{};
    engineTable[0x12c / 4] = reinterpret_cast<uintptr_t>(solidMoved);
    engineTable[0x130 / 4] = reinterpret_cast<uintptr_t>(triggerMoved);
    Interface engine{engineTable, nullptr};
    global<Interface*>(0x10635ca8) = &engine;
    std::array<float, 3> previous{1.f, 2.f, 3.f};
    NativeTouchObservation observation{reply, actor.data(), edicts.data(),
        query.priorOrigin ? previous.data() : nullptr, query.accurate != 0};
    observed = &observation;
    reinterpret_cast<void (__thiscall*)(void*, void*)>(0x100cddb0)(actor.data(), observation.previousOrigin);
    observed = nullptr;
    reply.checkUntouch = cell<uint32_t>(actor.data(), 0x84);
    const bool queued = (cell<uint32_t>(actor.data(), 0x11c) & 0x1000000) != 0;
    if (queued != (query.alreadyQueued || reply.checkUntouch)) {
        throw std::runtime_error("unexpected check-untouch flag");
    }
    return reply;
}

#else
Reply run(const Query& query) {
    const bool mapped = query.clientPresent && sourceMapToolClientMappingCandidate(
        query.handle, query.serverSerial, query.directSlot == 2);
    const bool local = sourceMapToolLocalPlayerCandidate(
        query.maxClients, query.localSlot != 0, query.localSlot == 1, query.localSlot == 4);
    const bool noclip = sourceMapToolReportsNoclipCandidate(mapped || local,
        static_cast<uint8_t>(mapped ? query.directMove : query.localMove));
    const auto scan = sourceMapTouchScanCandidate(query.hasEdict != 0, query.isWorld != 0,
        static_cast<uint8_t>(query.solidType), static_cast<uint16_t>(query.solidFlags), query.modelState != 0);
    const bool relink = scan != SourceMapTouchScanCandidate::none;
    return {mapped, local, noclip, (query.actorFlags & 0x80u) != 0, static_cast<uint32_t>(scan), relink,
        relink && !query.alreadyQueued, scan == SourceMapTouchScanCandidate::solidMoved && query.priorOrigin,
        relink && query.accurate};
}
#endif

int main(int argc, char** argv) {
    try {
        if (argc != 4) throw std::runtime_error("usage: trigger-actor queries replies reference-or-dash");
        std::ifstream input(argv[1], std::ios::binary);
        std::ofstream output(argv[2], std::ios::binary);
        if (!input || !output) throw std::runtime_error("map trigger actor fixture I/O");
#ifdef NATIVE_MAP_TRIGGER_ACTOR
        NativePrivateWorldImageFixture image(argv[3], 0x6de6c4);
        image.seal();
        PrivateX86BoundaryObserverFixture observer{
            {0x10111000, 0x10002500, 0x10002600, 0x10002700}, boundary};
        observer.configure(true);
#endif
        Query query{};
        size_t count = 0;
        while (input.read(reinterpret_cast<char*>(&query), sizeof(query))) {
            const auto reply = run(query);
            output.write(reinterpret_cast<const char*>(&reply), sizeof(reply));
            ++count;
        }
        if (!input.eof() || input.gcount() || !output || !count) {
            throw std::runtime_error("invalid trigger actor queries");
        }
#ifdef NATIVE_MAP_TRIGGER_ACTOR
        observer.configure(false);
        image.verify();
#endif
        std::cout << "map trigger actor queries=" << count << '\n';
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
