#include "source_solid_trigger_scan_candidate.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

using Point = SourceTriggerPointCandidate;
using Bounds = SourceTriggerBoundsCandidate;
using Sweep = SourceSolidTriggerSweepCandidate;
using Target = SourceSolidTriggerTargetCandidate;

struct Query {
    Point origin, previous;
    Bounds local, surrounding;
    uint32_t hasPrevious, solidType, accurate, blockedFlags, count;
};

struct Entry {
    uint32_t id, flags, state, contents;
    Bounds bounds;
};

struct Reply {
    Point center{}, delta{}, offset{}, extents{};
    uint32_t swept = 0, isRay = 0, queryKind = 0, clipCalls = 0, count = 0;
    std::array<uint32_t, 32> contacts{};
};

static_assert(sizeof(Query) == 92 && sizeof(Entry) == 40 && sizeof(Reply) == 196);

void capture(Reply& reply, const Sweep& sweep) {
    reply.center = sweep.center;
    reply.delta = sweep.delta;
    reply.offset = sweep.offset;
    reply.extents = sweep.extents;
    reply.swept = sweep.swept;
    reply.isRay = sweep.isRay;
}

#ifdef NATIVE_SOLID_TRIGGER_SCAN
#include <windows.h>
#include "native_private_world_image_fixture.h"

template<typename T>
T& global(uintptr_t address) {
    return *reinterpret_cast<T*>(address);
}

struct BoxRay {
    std::array<float, 4> center, delta, offset, extents;
    uint32_t worldTransform;
    uint8_t isRay, swept;
};
static_assert(offsetof(BoxRay, swept) == 0x45);

struct FixtureObject {
    const uintptr_t* table;
    void* payload;
};

struct FixtureEdict {
    uint32_t flags, unused1, unused2;
    FixtureObject* networkable;
    uint32_t id;
};

struct FixtureCollideable {
    const uintptr_t* table;
    FixtureObject* unknown;
    const Entry* target;
};

struct Observation {
    const Query& query;
    Reply& reply;
    FixtureEdict* mover;
    std::vector<FixtureCollideable*> candidates;
    bool enumerated = false;
};
static Observation* observed;

const Point* __fastcall getLocalMin(void*, void*) { return &observed->query.local.min; }
const Point* __fastcall getLocalMax(void*, void*) { return &observed->query.local.max; }
const Point* __fastcall getOrigin(void*, void*) { return &observed->query.origin; }
uint32_t __fastcall getSolidType(void*, void*) { return observed->query.solidType; }
bool __fastcall shouldTouch(void*, void*, uint32_t flags) { return !(flags & observed->query.blockedFlags); }

void __fastcall getSurrounding(void*, void*, Point* min, Point* max) {
    *min = observed->query.surrounding.min;
    *max = observed->query.surrounding.max;
}

void __fastcall getTriggerBounds(FixtureCollideable* c, void*, Point* min, Point* max) {
    *min = c->target->bounds.min;
    *max = c->target->bounds.max;
}

FixtureObject* __fastcall getUnknown(FixtureCollideable* c, void*) { return c->unknown; }
void* __fastcall getPayload(FixtureObject* object, void*) { return object->payload; }
uint32_t __fastcall getSolidFlags(FixtureCollideable* c, void*) { return c->target->flags; }
void* __fastcall allocate(void*, void*, size_t count) { return std::malloc(count); }
void* __fastcall reallocate(void*, void*, void* ptr, size_t count) { return std::realloc(ptr, count); }
void __fastcall release(void*, void*, void* ptr) { std::free(ptr); }

void dispatchCandidates(void* enumerator) {
    using Enumerate = uint32_t (__thiscall*)(void*, void*);
    const auto entry = **reinterpret_cast<const uintptr_t* const*>(enumerator);
    for (auto* candidate : observed->candidates) {
        if (reinterpret_cast<Enumerate>(entry)(enumerator, candidate) != 0) {
            throw std::runtime_error("unexpected enumeration stop");
        }
    }
    observed->enumerated = true;
}

void observeQuery(uint32_t listMask, uint32_t coarse, uint32_t kind, const void* enumerator) {
    if (listMask != 2 || coarse || observed->reply.queryKind ||
        **reinterpret_cast<const uintptr_t* const*>(enumerator) != 0x101dbd50) {
        throw std::runtime_error("unexpected trigger query");
    }
    const auto& ray = *reinterpret_cast<const BoxRay*>(static_cast<const uint8_t*>(enumerator) + 0x10);
    if (ray.worldTransform != 0) throw std::runtime_error("unexpected world transform");
    Sweep sweep{};
    for (std::size_t axis = 0; axis < 3; ++axis) {
        sweep.center[axis] = ray.center[axis];
        sweep.delta[axis] = ray.delta[axis];
        sweep.offset[axis] = ray.offset[axis];
        sweep.extents[axis] = ray.extents[axis];
    }
    sweep.swept = ray.swept != 0;
    sweep.isRay = ray.isRay != 0;
    capture(observed->reply, sweep);
    observed->reply.queryKind = kind;
}

void __fastcall enumerateBox(void*, void*, uint32_t mask, const Point* min, const Point* max,
                             uint32_t coarse, void* enumerator) {
    const auto& bounds = observed->query.surrounding;
    if (*min != bounds.min || *max != bounds.max) throw std::runtime_error("wrong query bounds");
    observeQuery(mask, coarse, 1, enumerator);
    dispatchCandidates(enumerator);
}

void __fastcall enumerateRay(void*, void*, uint32_t mask, const BoxRay* ray,
                             uint32_t coarse, void* enumerator) {
    if (ray != reinterpret_cast<const BoxRay*>(static_cast<const uint8_t*>(enumerator) + 0x10)) {
        throw std::runtime_error("wrong sweep pointer");
    }
    observeQuery(mask, coarse, 2, enumerator);
    dispatchCandidates(enumerator);
}

void __fastcall clip(void*, void*, const BoxRay*, uint32_t mask, FixtureCollideable* c, void* trace) {
    if (mask != 0x0200400b || observed->enumerated) throw std::runtime_error("unexpected brush trace");
    ++observed->reply.clipCalls;
    std::memcpy(static_cast<uint8_t*>(trace) + 0x30, &c->target->contents, sizeof(mask));
}

void __fastcall markContact(void*, void*, FixtureEdict* trigger, FixtureEdict* mover) {
    auto& reply = observed->reply;
    if (!observed->enumerated || mover != observed->mover || reply.count >= reply.contacts.size()) {
        throw std::runtime_error("unexpected trigger contact delivery");
    }
    reply.contacts[reply.count++] = trigger->id;
}

Reply run(const Query& query, const std::vector<Entry>& entries) {
    Reply reply{};
    uintptr_t networkableTable[4]{};
    networkableTable[3] = reinterpret_cast<uintptr_t>(getPayload);
    uintptr_t unknownTable[3]{};
    unknownTable[2] = reinterpret_cast<uintptr_t>(getPayload);
    uintptr_t moverTable[17]{};
    moverTable[1] = reinterpret_cast<uintptr_t>(getLocalMin);
    moverTable[2] = reinterpret_cast<uintptr_t>(getLocalMax);
    moverTable[8] = reinterpret_cast<uintptr_t>(getOrigin);
    moverTable[11] = reinterpret_cast<uintptr_t>(getSolidType);
    moverTable[15] = reinterpret_cast<uintptr_t>(getSurrounding);
    moverTable[16] = reinterpret_cast<uintptr_t>(shouldTouch);
    FixtureCollideable moverBox{moverTable, nullptr, nullptr};
    FixtureObject moverNetworkable{networkableTable, &moverBox};
    FixtureEdict moverEdict{4, 0, 0, &moverNetworkable, 0};
    Observation observation{query, reply, &moverEdict, {}};
    observed = &observation;

    uintptr_t candidateTable[13]{};
    candidateTable[3] = reinterpret_cast<uintptr_t>(getTriggerBounds);
    candidateTable[4] = reinterpret_cast<uintptr_t>(getUnknown);
    candidateTable[12] = reinterpret_cast<uintptr_t>(getSolidFlags);
    std::array<FixtureObject, 32> networkables{}, unknowns{};
    std::array<FixtureCollideable, 32> candidates{};
    std::array<FixtureEdict, 32> edicts{};
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto& entry = entries[i];
        candidates[i] = {candidateTable, &unknowns[i], &entry};
        networkables[i] = {networkableTable, &candidates[i]};
        edicts[i] = {entry.state == 2 ? 0u : 4u, 0, 0, entry.state == 3 ? nullptr : &networkables[i], entry.id};
        unknowns[i] = {unknownTable, entry.state == 1 ? &moverEdict : &edicts[i]};
        observation.candidates.push_back(&candidates[i]);
    }

    uintptr_t allocatorTable[6]{};
    allocatorTable[1] = reinterpret_cast<uintptr_t>(allocate);
    allocatorTable[3] = reinterpret_cast<uintptr_t>(reallocate);
    allocatorTable[5] = reinterpret_cast<uintptr_t>(release);
    FixtureObject allocator{allocatorTable, nullptr};
    auto* allocatorPointer = &allocator;
    global<FixtureObject**>(0x1031a4e8) = &allocatorPointer;

    uintptr_t partitionTable[15]{};
    partitionTable[12] = reinterpret_cast<uintptr_t>(enumerateBox);
    partitionTable[14] = reinterpret_cast<uintptr_t>(enumerateRay);
    global<const uintptr_t*>(0x106683c0) = partitionTable;

    uintptr_t traceTable[5]{};
    traceTable[4] = reinterpret_cast<uintptr_t>(clip);
    FixtureObject traceService{traceTable, nullptr};
    global<FixtureObject*>(0x10407e8c) = &traceService;
    uintptr_t callbackTable[2]{};
    callbackTable[1] = reinterpret_cast<uintptr_t>(markContact);
    FixtureObject callbacks{callbackTable, nullptr};
    global<FixtureObject*>(0x1066ca88) = &callbacks;

    // Enter through the same VEngineServer022 method called by game code.
    using SolidMoved = void (__thiscall*)(void*, FixtureEdict*, FixtureCollideable*, const Point*, uint32_t);
    const auto entry = global<uintptr_t>(global<uintptr_t>(0x10402a1c) + 0x12c);
    reinterpret_cast<SolidMoved>(entry)(reinterpret_cast<void*>(0x10402a1c), &moverEdict, &moverBox,
                                       query.hasPrevious ? &query.previous : nullptr, query.accurate);
    observed = nullptr;
    if (!observation.enumerated) throw std::runtime_error("missing trigger query");
    return reply;
}

#else
Reply run(const Query& query, const std::vector<Entry>& entries) {
    Reply reply{};
    const auto sweep = sourceSolidTriggerSweepCandidate(query.origin,
        query.hasPrevious ? std::optional<Point>{query.previous} : std::nullopt,
        query.solidType, query.accurate != 0, query.local, query.surrounding);
    capture(reply, sweep);
    reply.queryKind = query.hasPrevious ? 2u : 1u;
    std::vector<Target> targets;
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto& e = entries[i];
        targets.push_back({static_cast<uint32_t>(i), e.flags, e.state == 1, e.state != 2, e.state != 3, e.bounds});
    }
    const auto contacts = sourceSolidTriggerContactsCandidate(sweep, targets,
        [&](uint32_t flags) { return !(flags & query.blockedFlags); },
        [&](const Sweep&, uint32_t index, uint32_t) {
            ++reply.clipCalls;
            return entries[index].contents;
        });
    reply.count = static_cast<uint32_t>(contacts.size());
    for (std::size_t i = 0; i < contacts.size(); ++i) reply.contacts[i] = entries[contacts[i]].id;
    return reply;
}
#endif

int main(int argc, char** argv) {
    try {
        if (argc != 4) throw std::runtime_error("usage: solid-trigger-scan queries replies reference-or-dash");
        std::ifstream input(argv[1], std::ios::binary);
        std::ofstream output(argv[2], std::ios::binary);
        if (!input || !output) throw std::runtime_error("solid trigger scan fixture I/O");
#ifdef NATIVE_SOLID_TRIGGER_SCAN
        NativePrivateWorldImageFixture image(argv[3], 0x698000);
        image.seal();
#endif
        Query query{};
        std::size_t count = 0;
        while (input.read(reinterpret_cast<char*>(&query), sizeof(query))) {
            if (query.count > Reply{}.contacts.size()) throw std::runtime_error("fixture candidate limit");
            std::vector<Entry> entries(query.count);
            const auto bytes = static_cast<std::streamsize>(entries.size() * sizeof(Entry));
            if (bytes && !input.read(reinterpret_cast<char*>(entries.data()), bytes)) {
                throw std::runtime_error("incomplete fixture candidates");
            }
            const auto reply = run(query, entries);
            output.write(reinterpret_cast<const char*>(&reply), sizeof(reply));
            ++count;
        }
        if (!input.eof() || input.gcount() || !output || !count) throw std::runtime_error("invalid scan queries");
#ifdef NATIVE_SOLID_TRIGGER_SCAN
        image.verify();
#endif
        std::cout << "solid trigger scan queries=" << count << '\n';
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
