#include "source_trigger_activation_scan_candidate.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>

using Point = SourceTriggerPointCandidate;
using Bounds = SourceTriggerBoundsCandidate;
using Sweep = SourceSolidTriggerSweepCandidate;

struct Query {
    Point origin;
    Bounds local, triggerBounds;
    uint32_t flags, accurate, disabled, hasEdict, isWorld, hasModel, solidType, count;
};

struct Entry {
    uint32_t id, excluded, type, contents;
    Point origin;
    Bounds local, surrounding;
};

struct Reply {
    Bounds box{};
    uint32_t listMask = 0, scan = 0, clipCalls = 0, clipHash = 0, count = 0;
    std::array<uint32_t, 32> contacts{};
};
static_assert(sizeof(Query) == 92 && sizeof(Entry) == 76 && sizeof(Reply) == 172);

enum Exclusion : uint32_t {
    staticProp = 1, sameTrigger = 2, filterRejected = 4, notServerEntity = 8, noNetworkable = 16
};

Bounds scanBox(const Query& query) {
    if (query.flags & 0x80) return query.triggerBounds;
    Bounds result{};
    for (std::size_t axis = 0; axis < 3; ++axis) {
        result.min[axis] = query.local.min[axis] + query.origin[axis];
        result.max[axis] = query.local.max[axis] + query.origin[axis];
    }
    return result;
}

void recordClip(Reply& reply, const Sweep& hull, uint32_t id) {
    ++reply.clipCalls;
    auto add = [&](uint32_t word) { reply.clipHash = (reply.clipHash ^ word) * 16777619u; };
    add(id);
    for (const auto& point : {hull.center, hull.delta, hull.offset, hull.extents}) {
        for (float value : point) {
            uint32_t bits;
            std::memcpy(&bits, &value, sizeof(bits));
            add(bits);
        }
    }
    add(hull.isRay);
    add(hull.swept);
}

#ifdef NATIVE_TRIGGER_ACTIVATION_SCAN
#include <windows.h>
#include "native_private_world_image_fixture.h"

template<typename T>
T& cell(void* base, std::size_t offset) {
    return *reinterpret_cast<T*>(static_cast<uint8_t*>(base)+offset);
}

template<typename T>
T& global(uintptr_t address) {
    return *reinterpret_cast<T*>(address);
}

struct Interface {
    const uintptr_t* table;
    void* value;
};
void* __fastcall getValue(Interface* item, void*) { return item->value; }
void* __fastcall allocate(void*, void*, std::size_t size) { return std::malloc(size); }
void* __fastcall reallocate(void*, void*, void* ptr, std::size_t size) { return std::realloc(ptr, size); }
void __fastcall release(void*, void*, void* ptr) { std::free(ptr); }

struct NativeRay {
    std::array<float, 4> center, delta, offset, extents;
    uint32_t transform;
    uint8_t isRay, swept;
};
static_assert(offsetof(NativeRay, swept) == 0x45);

struct Edict {
    uint32_t flags, unused1, unused2;
    Interface* networkable;
    uint32_t id;
};

struct Collision {
    const uintptr_t* table;
    Point origin;
    Bounds local, surrounding, triggerBounds;
    uint32_t flags, type;
    bool accepts;
};

struct Candidate {
    const uintptr_t* table;
    Edict* edict;
    Collision* collision;
    Interface* networkable;
    uint32_t handle;
};

struct Observation {
    const Query& query;
    Reply& reply;
    Edict* triggerEdict = nullptr;
    Collision* trigger = nullptr;
    const std::vector<Entry>* entries = nullptr;
    std::vector<Candidate> candidates;
    const Entry* active = nullptr;
    bool collected = false;
};
static Observation* observed;

const Point* __fastcall getOrigin(Collision* c, void*) { return &c->origin; }
const Point* __fastcall getMin(Collision* c, void*) { return &c->local.min; }
const Point* __fastcall getMax(Collision* c, void*) { return &c->local.max; }
const Point* __fastcall getAngles(void*, void*) { static const Point zero{}; return &zero; }
uint32_t __fastcall getFlags(Collision* c, void*) { return c->flags; }
uint32_t __fastcall getType(Collision* c, void*) { return c->type; }
const uint32_t* __fastcall getHandle(Candidate* c, void*) { return &c->handle; }
Collision* __fastcall getCollision(Candidate* c, void*) { return c->collision; }
Interface* __fastcall getNetworkable(Candidate* c, void*) { return c->networkable; }

void __fastcall getBounds(Collision* c, void*, Point* min, Point* max) {
    *min = c->surrounding.min;
    *max = c->surrounding.max;
}
void __fastcall getTriggerBounds(Collision* c, void*, Point* min, Point* max) {
    *min = c->triggerBounds.min;
    *max = c->triggerBounds.max;
}
bool __fastcall acceptsTrigger(Collision* c, void*, uint32_t flags) {
    if (flags != observed->query.flags) throw std::runtime_error("wrong trigger flags passed to solid");
    return c->accepts;
}

void __fastcall enumerateBox(void*, void*, uint32_t mask, const Point* min, const Point* max,
                            uint32_t coarse, void* enumerator) {
    const auto entry = **reinterpret_cast<const uintptr_t* const*>(enumerator);
    if (mask != 1 || coarse || entry != 0x101dbfb0 || observed->reply.listMask) {
        throw std::runtime_error("unexpected solid partition query");
    }
    observed->reply.listMask = mask;
    observed->reply.box = {*min,*max};
    using Enumerate = uint32_t (__thiscall*)(void*, void*);
    for (std::size_t i = 0; i < observed->candidates.size(); ++i) {
        observed->active = &observed->entries->at(i);
        if (reinterpret_cast<Enumerate>(entry)(enumerator, &observed->candidates[i]) != 0) {
            throw std::runtime_error("unexpected enumeration stop");
        }
    }
    observed->active = nullptr;
    observed->collected = true;
}

void __fastcall clip(void*, void*, const NativeRay* ray, uint32_t mask, Collision* target, void* trace) {
    if (mask != 0x0200400b || !observed->active || target != observed->trigger || ray->transform) {
        throw std::runtime_error("unexpected reverse-scan clip");
    }
    const auto point = [](const std::array<float, 4>& v) { return Point{v[0],v[1],v[2]}; };
    recordClip(observed->reply, {point(ray->center), point(ray->delta), point(ray->offset),
        point(ray->extents), ray->isRay!=0, ray->swept!=0}, observed->active->id);
    std::memcpy(static_cast<uint8_t*>(trace)+0x30, &observed->active->contents, sizeof(mask));
}

void __fastcall markContact(void*, void*, Edict* solid, Edict* trigger) {
    auto& reply = observed->reply;
    if (!observed->collected || trigger != observed->triggerEdict || reply.count >= reply.contacts.size()) {
        throw std::runtime_error("unexpected contact argument or order");
    }
    reply.contacts[reply.count++] = solid->id;
}

Reply runEngine(const Query& query, const std::vector<Entry>& entries) {
    Reply reply{};
    uintptr_t collisionTable[17]{};
    collisionTable[1] = reinterpret_cast<uintptr_t>(getMin);
    collisionTable[2] = reinterpret_cast<uintptr_t>(getMax);
    collisionTable[3] = reinterpret_cast<uintptr_t>(getTriggerBounds);
    collisionTable[8] = reinterpret_cast<uintptr_t>(getOrigin);
    collisionTable[9] = reinterpret_cast<uintptr_t>(getAngles);
    collisionTable[11] = reinterpret_cast<uintptr_t>(getType);
    collisionTable[12] = reinterpret_cast<uintptr_t>(getFlags);
    collisionTable[15] = reinterpret_cast<uintptr_t>(getBounds);
    collisionTable[16] = reinterpret_cast<uintptr_t>(acceptsTrigger);
    Collision trigger{collisionTable, query.origin, query.local, {}, query.triggerBounds,
        query.flags, query.solidType, true};
    uintptr_t networkableTable[4]{};
    networkableTable[2] = reinterpret_cast<uintptr_t>(getValue);
    networkableTable[3] = reinterpret_cast<uintptr_t>(getValue);
    Interface triggerNetworkable{networkableTable, &trigger};
    Edict triggerEdict{4, 0, 0, &triggerNetworkable, 0};
    Observation observation{query, reply, &triggerEdict, &trigger, &entries, {}};
    observed = &observation;

    uintptr_t handleTable[5]{};
    handleTable[2] = reinterpret_cast<uintptr_t>(getHandle);
    handleTable[3] = reinterpret_cast<uintptr_t>(getCollision);
    handleTable[4] = reinterpret_cast<uintptr_t>(getNetworkable);
    std::array<Edict, 32> solids{};
    std::array<Collision, 32> boxes{};
    std::array<Interface, 32> networkables{}, unknowns{};
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto& e = entries[i];
        boxes[i] = {collisionTable, e.origin, e.local, e.surrounding, {}, 0, e.type,
            !(e.excluded & filterRejected)};
        solids[i] = {e.excluded & notServerEntity ? 0u : 4u, 0, 0,
            e.excluded & noNetworkable ? nullptr : &networkables[i], e.id};
        unknowns[i] = {networkableTable, &solids[i]};
        networkables[i] = {networkableTable, &boxes[i]};
        observation.candidates.push_back({handleTable, &solids[i],
            e.excluded & sameTrigger ? &trigger : &boxes[i], &unknowns[i],
            (e.excluded & staticProp ? 0x40000000u : 0u) | static_cast<uint32_t>(i)});
    }

    uintptr_t allocatorTable[6]{};
    allocatorTable[1] = reinterpret_cast<uintptr_t>(allocate);
    allocatorTable[3] = reinterpret_cast<uintptr_t>(reallocate);
    allocatorTable[5] = reinterpret_cast<uintptr_t>(release);
    Interface allocator{allocatorTable, nullptr};
    auto* allocatorPointer = &allocator;
    global<Interface**>(0x1031a4e8) = &allocatorPointer;
    uintptr_t partitionTable[13]{};
    partitionTable[12] = reinterpret_cast<uintptr_t>(enumerateBox);
    global<const uintptr_t*>(0x106683c0) = partitionTable;
    uintptr_t traceTable[5]{};
    traceTable[4] = reinterpret_cast<uintptr_t>(clip);
    Interface traceService{traceTable, nullptr};
    global<Interface*>(0x10407e8c) = &traceService;
    uintptr_t callbackTable[2]{};
    callbackTable[1] = reinterpret_cast<uintptr_t>(markContact);
    Interface callbacks{callbackTable, nullptr};
    global<Interface*>(0x1066ca88) = &callbacks;
    using TriggerMoved = void (__thiscall*)(void*, Edict*, uint32_t);
    const auto entry = global<uintptr_t>(global<uintptr_t>(0x10402a1c)+0x130);
    reinterpret_cast<TriggerMoved>(entry)(reinterpret_cast<void*>(0x10402a1c), &triggerEdict, query.accurate);
    observed = nullptr;
    if (!observation.collected) throw std::runtime_error("missing trigger-side enumerator");
    return reply;
}

uint32_t __fastcall serverSolidType(void* collision, void*) { return cell<uint8_t>(collision, 0x22); }
uint32_t __fastcall serverModelIndex(void*, void*) { return 1; }
void* __fastcall lookupModel(Interface* item, void*, uint32_t) { return item->value; }
const char** __fastcall getModelName(void*, void*, const char** name) { *name = nullptr; return name; }
void __cdecl discardWarning(const char*, ...) {}

void captureServerScan(void* edict, uint32_t accurate, SourceMapTouchScanCandidate scan) {
    auto& reply = observed->reply;
    if (edict != observed->triggerEdict || accurate != observed->query.accurate || reply.scan) {
        throw std::runtime_error("unexpected server activation scan");
    }
    reply.scan = static_cast<uint32_t>(scan);
}

void __fastcall serverTriggerMoved(void*, void*, void* edict, uint32_t accurate) {
    captureServerScan(edict, accurate, SourceMapTouchScanCandidate::triggerMoved);
}
void __fastcall serverSolidMoved(void*, void*, void* edict, void*, void* previous, uint32_t accurate) {
    if (previous) throw std::runtime_error("client activation included previous origin");
    captureServerScan(edict, accurate, SourceMapTouchScanCandidate::solidMoved);
}

Reply runServer(const Query& query) {
    Reply reply{};
    alignas(4) std::array<uint8_t, 0x430> trigger{};
    Edict edict{};
    uintptr_t actorTable[8]{};
    actorTable[0x18 / 4] = reinterpret_cast<uintptr_t>(serverModelIndex);
    actorTable[0x1c / 4] = reinterpret_cast<uintptr_t>(getModelName);
    uintptr_t collisionTable[12]{};
    collisionTable[0x2c / 4] = reinterpret_cast<uintptr_t>(serverSolidType);
    cell<const uintptr_t*>(trigger.data(), 0) = actorTable;
    cell<const uintptr_t*>(trigger.data(), 0x164) = collisionTable;
    cell<Edict*>(trigger.data(), 0x18) = query.hasEdict ? &edict : nullptr;
    cell<uint8_t>(trigger.data(), 0x420) = static_cast<uint8_t>(query.disabled);
    cell<uint16_t>(trigger.data(), 0x184) = static_cast<uint16_t>(query.flags);
    cell<uint8_t>(trigger.data(), 0x186) = static_cast<uint8_t>(query.solidType);
    // The existing actor probe checks insertion onto the untouch work list.
    // Here supply an already scheduled actor to isolate the activation gate.
    cell<uint32_t>(trigger.data(), 0x11c) = 0x1000000u;
    uintptr_t modelsTable[2]{};
    modelsTable[1] = reinterpret_cast<uintptr_t>(lookupModel);
    Interface modelInfo{modelsTable, query.hasModel ? trigger.data() : nullptr};
    global<Interface*>(0x10635cc8) = &modelInfo;
    global<decltype(&discardWarning)>(0x1043c234) = discardWarning;
    global<void*>(0x106716d4) = query.isWorld ? trigger.data() : nullptr;
    global<uint8_t>(0x105938fc) = static_cast<uint8_t>(query.accurate);
    uintptr_t engineTable[0x130 / 4 + 1]{};
    engineTable[0x12c / 4] = reinterpret_cast<uintptr_t>(serverSolidMoved);
    engineTable[0x130 / 4] = reinterpret_cast<uintptr_t>(serverTriggerMoved);
    Interface engine{engineTable, nullptr};
    global<Interface*>(0x10635ca8) = &engine;
    Observation observation{query, reply, &edict, nullptr, nullptr, {}};
    observed = &observation;
    const auto entry = global<uintptr_t>(0x104e2a2c+0x74);
    reinterpret_cast<void (__thiscall*)(void*)>(entry)(trigger.data());
    observed = nullptr;
    return reply;
}

#else
Reply runEngine(const Query& query, const std::vector<Entry>& entries) {
    Reply reply{};
    reply.box = scanBox(query);
    reply.listMask = 1;
    std::vector<SourceTriggerSolidCandidate> candidates;
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto& e = entries[i];
        candidates.push_back({static_cast<uint32_t>(i), (e.excluded & staticProp)!=0,
            (e.excluded & sameTrigger)!=0, !(e.excluded & filterRejected), !(e.excluded & notServerEntity),
            !(e.excluded & noNetworkable), e.origin, e.type, e.local, e.surrounding});
    }
    const auto contacts = sourceTriggerMovedContactsCandidate(query.flags, query.accurate!=0,
        query.triggerBounds, candidates, [&](const Sweep& hull, uint32_t i, uint32_t) {
            recordClip(reply, hull, entries[i].id);
            return entries[i].contents;
        });
    reply.count = static_cast<uint32_t>(contacts.size());
    for (std::size_t i = 0; i < contacts.size(); ++i) reply.contacts[i] = entries[contacts[i]].id;
    return reply;
}

Reply runServer(const Query& query) {
    Reply reply{};
    reply.scan = static_cast<uint32_t>(sourcePostClientActiveScanCandidate(
        query.disabled!=0, query.hasEdict!=0, query.isWorld!=0, static_cast<uint8_t>(query.solidType),
        static_cast<uint16_t>(query.flags), query.hasModel!=0));
    return reply;
}
#endif

int main(int argc, char** argv) {
    try {
        if (argc != 5) throw std::runtime_error("usage: activate-trigger engine|server queries replies reference-or-dash");
        const std::string mode = argv[1];
        if (mode != "engine" && mode != "server") throw std::runtime_error("invalid fixture mode");
        std::ifstream input(argv[2], std::ios::binary);
        std::ofstream output(argv[3], std::ios::binary);
        if (!input || !output) throw std::runtime_error("activation fixture I/O");
#ifdef NATIVE_TRIGGER_ACTIVATION_SCAN
        NativePrivateWorldImageFixture image(argv[4], mode=="server" ? 0x6de6c4u : 0x698000u);
        image.seal();
#endif
        Query query{};
        std::size_t count = 0;
        while (input.read(reinterpret_cast<char*>(&query), sizeof(query))) {
            if (query.count > Reply{}.contacts.size()) throw std::runtime_error("fixture candidate limit");
            std::vector<Entry> entries(query.count);
            const auto bytes = static_cast<std::streamsize>(entries.size()*sizeof(Entry));
            if (bytes && !input.read(reinterpret_cast<char*>(entries.data()), bytes)) {
                throw std::runtime_error("incomplete activation candidates");
            }
            const auto reply = mode=="server" ? runServer(query) : runEngine(query, entries);
            output.write(reinterpret_cast<const char*>(&reply), sizeof(reply));
            ++count;
        }
        if (!input.eof() || input.gcount() || !output || !count) throw std::runtime_error("invalid activation queries");
#ifdef NATIVE_TRIGGER_ACTIVATION_SCAN
        image.verify();
#endif
        std::cout << mode << " trigger activation queries=" << count << '\n';
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
