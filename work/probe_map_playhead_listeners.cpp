// Research fixture only: original listener bodies run in a private x86 image.
// UI services and explicitly named callees are observed, not executed.
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
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
uintptr_t base = 0, document = 0, target = 0;
unsigned updates = 0, virtualUpdates = 0, begins = 0, ends = 0;
std::array<unsigned, 4> boundaries{};
int32_t suppliedTime = 0;
bool viewport = false;
bool cleanupMode = false;
uintptr_t cleanupElement = 0;
unsigned elementStatus = 0;
std::vector<uintptr_t> freedNodes;
struct FilterNode { uintptr_t listener; uint32_t mask; FilterNode* next; };
struct TrackedRecord { uint32_t element, kind, symbol; };
bool subscribeMode = false;
unsigned allocationCalls = 0, failAllocation = 0, insertedRecords = 0;
std::array<FilterNode, 6> subscriptionNodes{};
std::array<TrackedRecord, 6> subscriptionRecords{};
std::array<uintptr_t, 6> subscriptionElements{};
uintptr_t activeClip = 0;
LONG CALLBACK diagnoseFailure(EXCEPTION_POINTERS* p) {
    if (p->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
        std::cerr << "ACCESS_VIOLATION eip=" << std::hex << p->ContextRecord->Eip
                  << " allocation=" << allocationCalls << " records=" << insertedRecords << '\n';
    return EXCEPTION_CONTINUE_SEARCH;
}

void __fastcall update(void* self, void*) {
    require(reinterpret_cast<uintptr_t>(self) == target, "unexpected QWidget update target");
    ++updates;
}
void __fastcall refreshTrack(void* self, void*) {
    require(reinterpret_cast<uintptr_t>(self) == base, "unexpected track virtual owner");
    ++virtualUpdates;
}
void __fastcall positionOverlay(void* self, void*, int32_t time) {
    require(reinterpret_cast<uintptr_t>(self) == base && time == (document ? suppliedTime : 0),
        "unexpected overlay virtual argument");
    ++virtualUpdates;
}
void __fastcall beginProfile(void*, void*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) { ++begins; }
void __fastcall endProfile(void*, void*) { ++ends; }
void boundary(unsigned slot, CONTEXT& c) {
    using Observer = PrivateX86BoundaryObserverFixture;
    ++boundaries.at(slot);
    if (subscribeMode) {
        using Observer = PrivateX86BoundaryObserverFixture;
        if (slot == 0) {
            require(c.Ecx == base, "subscription UI owner");
            Observer::returnCdecl(c);
        } else if (slot == 1) {
            require(c.Ecx == 0x10f49a28 && at<uint32_t>(c.Esp+4) == 12, "subscription pool allocation");
            require(allocationCalls < subscriptionNodes.size(), "subscription allocation overflow");
            auto* node = &subscriptionNodes[allocationCalls++];
            Observer::returnCdecl(c, allocationCalls == failAllocation ? 0 : reinterpret_cast<uintptr_t>(node));
            c.Esp += 4;
        } else if (slot == 2) {
            require(c.Ecx == base+0x3c && at<uint32_t>(c.Esp+4) == insertedRecords, "subscription insertion owner/index");
            require(insertedRecords < subscriptionRecords.size(), "subscription record overflow");
            subscriptionRecords[insertedRecords++] = at<TrackedRecord>(at<uint32_t>(c.Esp+8));
            at<uint32_t>(base+0x48) = insertedRecords;
            Observer::returnCdecl(c);
            c.Esp += 8;
        } else {
            require(c.Ecx == document, "active clip getter owner");
            Observer::returnCdecl(c, activeClip);
        }
        return;
    }
    if (cleanupMode && slot == 1) {
        require(c.Ecx == 0x10f49a28, "unexpected filtered listener pool");
        freedNodes.push_back(at<uint32_t>(c.Esp + 4));
        Observer::returnCdecl(c);
        c.Esp += 4;
        return;
    }
    if (viewport) {
        require(c.Ecx == (slot < 2 ? base + (slot == 1 ? 0x38 : 0) : document),
            "unexpected viewport helper owner");
        Observer::returnCdecl(c); // Root/active clip getters return null.
    } else {
        require(slot == 0 && c.Ecx == document, "unexpected time getter boundary");
        const auto result = at<uint32_t>(c.Esp + 4);
        at<int32_t>(result) = suppliedTime;
        Observer::returnCdecl(c, result);
        c.Esp += 4;
    }
}

void* __fastcall resolveSubscriptionElement(void*, void*, uint32_t handle) {
    require(handle >= 1 && handle <= subscriptionElements.size(), "subscription handle");
    return reinterpret_cast<void*>(subscriptionElements[handle-1]);
}
void* __fastcall getSubscriptionChild(void* self, void*) {
    // The preceding push 2 belongs to the later subscription helper. This
    // getter takes no argument and must leave that mask on the stack.
    return reinterpret_cast<void*>(at<uintptr_t>(reinterpret_cast<uintptr_t>(self)+0x80));
}

unsigned runSubscriptions() {
    subscribeMode = viewport = true;
    PrivateX86BoundaryObserverFixture observer{
        {0x101afc40, 0x1097aa70, 0x1008f4d0, 0x1029aae0}, boundary};
    observer.configure(true);
    unsigned cases = 0;
    for (unsigned active = 0; active < 3; ++active)
    for (unsigned children = 0; children < 16; ++children)
    for (unsigned failure = 0; failure < 7; ++failure) {
        alignas(4) std::array<uint8_t, 0x400> object{};
        alignas(4) std::array<uint8_t, 0x400> tool{};
        alignas(4) std::array<uint8_t, 0x100> doc{};
        std::array<std::array<uint8_t, 0xc0>, 6> elements{};
        std::array<uintptr_t, 0x48 / 4 + 1> modelTable{};
        std::array<uintptr_t, 0x5c / 4 + 1> clipTable{};
        modelTable[0x48 / 4] = reinterpret_cast<uintptr_t>(resolveSubscriptionElement);
        clipTable[0x5c / 4] = reinterpret_cast<uintptr_t>(getSubscriptionChild);
        const auto model = reinterpret_cast<uintptr_t>(modelTable.data());
        base = reinterpret_cast<uintptr_t>(object.data());
        document = reinterpret_cast<uintptr_t>(doc.data());
        for (unsigned i = 0; i < elements.size(); ++i) {
            subscriptionElements[i] = reinterpret_cast<uintptr_t>(elements[i].data());
            at<uintptr_t>(subscriptionElements[i]) = reinterpret_cast<uintptr_t>(clipTable.data());
            at<uint32_t>(subscriptionElements[i]+0x1c) = i+1;
            at<uint32_t>(subscriptionElements[i]+0x40) = 0xc;
        }
        // Root and active clips, their time frames and virtual-slot-5c child.
        at<uint32_t>(subscriptionElements[0]+0x78) = 2;
        if (!(children & 1)) subscriptionElements[1] = 0;
        if (!(children & 2)) subscriptionElements[2] = 0;
        at<uintptr_t>(subscriptionElements[0]+0x80) = subscriptionElements[2];
        at<uint32_t>(subscriptionElements[0]+0xb8) = 3;
        at<uint32_t>(subscriptionElements[3]+0x78) = 5;
        if (!(children & 4)) subscriptionElements[4] = 0;
        if (!(children & 8)) subscriptionElements[5] = 0;
        at<uintptr_t>(subscriptionElements[3]+0x80) = subscriptionElements[5];
        at<uint32_t>(subscriptionElements[3]+0xb8) = 6;
        activeClip = active == 0 ? 0 : subscriptionElements[active == 1 ? 0 : 3];
        at<uint32_t>(document+0x84) = 1;
        const auto owner = reinterpret_cast<uintptr_t>(tool.data());
        const uintptr_t list[] = {base+0x18};
        at<uintptr_t>(base+0x18) = 0x10ac9f7c;
        at<uintptr_t>(base+0x38) = base;
        at<uintptr_t>(base+0x3c) = reinterpret_cast<uintptr_t>(subscriptionRecords.data());
        at<uintptr_t>(owner+0x344) = document;
        at<uintptr_t>(owner+0x370) = reinterpret_cast<uintptr_t>(list);
        at<uint32_t>(owner+0x37c) = 1;
        at<uintptr_t>(0x10f32568) = owner;
        at<uintptr_t>(0x10e9a51c) = reinterpret_cast<uintptr_t>(&model);
        allocationCalls = insertedRecords = 0; failAllocation = failure;
        subscriptionNodes.fill({}); subscriptionRecords.fill({});
        std::vector<std::pair<unsigned, uint32_t>> expected;
        auto add = [&](unsigned i, uint32_t mask) { if (subscriptionElements[i]) expected.push_back({i, mask}); };
        add(0,3); add(1,2); add(2,2);
        if (active) { const unsigned i = active == 1 ? 0 : 3; add(i,3); add(i+1,2); add(i+2,2); }
        begins = ends = 0; boundaries.fill(0);
        reinterpret_cast<void (__thiscall*)(void*, int32_t, uint32_t)>(0x102beef0)(tool.data(), 120, 0);
        require(allocationCalls == expected.size() && insertedRecords == expected.size(), "subscription count");
        require(at<uint32_t>(base+0x48) == expected.size(), "tracked count after subscription");
        require(begins == 2 && ends == 2 && boundaries[0] == 1 && boundaries[3] == 1, "subscription dispatch count");
        std::array<FilterNode*, 6> heads{};
        std::array<uint32_t, 6> masks{}; masks.fill(3);
        for (unsigned j = 0; j < expected.size(); ++j) {
            const auto i = expected[j].first; const auto mask = expected[j].second;
            require(subscriptionRecords[j].element == i+1 && subscriptionRecords[j].kind == 0 && subscriptionRecords[j].symbol == UINT32_MAX, "tracked subscription record");
            masks[i] &= mask;
            if (j+1 != failure) {
                require(subscriptionNodes[j].listener == base+0x24 && subscriptionNodes[j].mask == mask && subscriptionNodes[j].next == heads[i], "filtered subscription node");
                heads[i] = &subscriptionNodes[j];
            }
        }
        for (unsigned i = 0; i < elements.size(); ++i) if (subscriptionElements[i]) {
            require(at<FilterNode*>(subscriptionElements[i]+0x44) == heads[i] && at<uint32_t>(subscriptionElements[i]+0x40) == masks[i]*4, "filtered head/mask");
        }
        // Deliver the clip-changed callback through its actual installed vtable.
        // Its common information-refresh callee remains observed.
        const auto refreshBefore = boundaries[0];
        at<uintptr_t>(base+0x24) = 0x10ac9f60;
        reinterpret_cast<void (__thiscall*)(void*, uint32_t, uint32_t)>(at<uintptr_t>(0x10ac9f60))(
            reinterpret_cast<void*>(base+0x24), 37, 3);
        require(boundaries[0] == refreshBefore+1 && allocationCalls == expected.size() && insertedRecords == expected.size(), "changed callback routing");
        ++cases;
    }
    observer.configure(false);
    subscribeMode = false;
    return cases;
}

void* __fastcall resolveTrackedElement(void*, void*, uint32_t handle) {
    require(handle == 37, "unexpected tracked element handle");
    return elementStatus ? reinterpret_cast<void*>(cleanupElement) : nullptr;
}
bool __fastcall validateTrackedElement(void* self, void*, uint32_t schema) {
    require(reinterpret_cast<uintptr_t>(self) == cleanupElement && schema == 0x1234,
        "unexpected tracked element validation");
    return elementStatus == 2;
}

unsigned runTrackedCleanup() {
    // The actual viewport time callback now runs its nonempty cleanup helper
    // and the actual filtered-list removal body. Only pool deallocation and
    // unrelated viewport information refresh are observed. No object is freed.
    viewport = cleanupMode = true;
    PrivateX86BoundaryObserverFixture observer{
        {0x101afc40, 0x1097a990, 0x10278e90, 0x1029aae0}, boundary};
    observer.configure(true);
    unsigned cases = 0;
    for (unsigned status = 0; status < 3; ++status)
    for (unsigned ownerPresent = 0; ownerPresent < 2; ++ownerPresent)
    for (unsigned shape = 0; shape < 5; ++shape)
    for (unsigned records = 0; records < 3; ++records)
    for (unsigned kind : {0u, 5u})
    for (uint32_t packed : {0u, 0xa5a5e0f3u}) {
        alignas(4) std::array<uint8_t, 0x400> object{};
        alignas(4) std::array<uint8_t, 0x400> tool{};
        alignas(4) std::array<uint8_t, 0x60> element{};
        std::array<uintptr_t, 0x48 / 4 + 1> modelTable{};
        modelTable[0x48 / 4] = reinterpret_cast<uintptr_t>(resolveTrackedElement);
        const auto model = reinterpret_cast<uintptr_t>(modelTable.data());
        const uintptr_t elementTable[] = {0, 0, 0, 0, reinterpret_cast<uintptr_t>(validateTrackedElement)};
        struct Node { uintptr_t listener; uint32_t mask; Node* next; };
        std::array<Node, 3> nodes{};
        struct Record { uint32_t element, kind, symbol; };
        const std::array<Record, 2> tracked = {{{37, kind, 0}, {37, kind, 0}}};
        base = reinterpret_cast<uintptr_t>(object.data());
        document = 0;
        cleanupElement = reinterpret_cast<uintptr_t>(element.data());
        elementStatus = status;
        const auto key = ownerPresent ? base + 0x24 : 0;
        const unsigned nodeCount = shape ? 3 : 0;
        std::vector<unsigned> remaining;
        for (unsigned i = 0; i < nodeCount; ++i) {
            const bool match = (shape == 2 && i == 0) || (shape == 3 && i == 2) || (shape == 4 && i != 1);
            nodes[i] = {match ? key : base + 0x200, (i+1) % 4, i+1 < nodeCount ? &nodes[i+1] : nullptr};
            remaining.push_back(i);
        }
        at<uintptr_t>(cleanupElement) = reinterpret_cast<uintptr_t>(elementTable);
        at<uint32_t>(cleanupElement+0x40) = packed;
        at<Node*>(cleanupElement+0x44) = nodeCount ? &nodes[0] : nullptr;
        auto expectedElement = element;
        const auto originalNodes = nodes;
        at<uintptr_t>(base+0x18) = 0x10ac9f7c;
        at<uintptr_t>(base+0x38) = ownerPresent ? base : 0;
        at<uintptr_t>(base+0x3c) = reinterpret_cast<uintptr_t>(tracked.data());
        at<uint32_t>(base+0x48) = records;
        const auto owner = reinterpret_cast<uintptr_t>(tool.data());
        const uintptr_t list[] = {base+0x18};
        at<uintptr_t>(owner+0x370) = reinterpret_cast<uintptr_t>(list);
        at<uint32_t>(owner+0x37c) = 1;
        at<uintptr_t>(0x10f32568) = owner;
        at<uintptr_t>(0x10e9a51c) = reinterpret_cast<uintptr_t>(&model);
        at<uint32_t>(0x10e9a684) = 0x1234;
        auto expectedObject = object;
        at<uint32_t>(reinterpret_cast<uintptr_t>(expectedObject.data())+0x48) = 0;
        std::vector<uintptr_t> expectedFree;
        uint32_t expectedPacked = packed;
        if (status == 2 && kind == 0) {
            for (unsigned record = 0; record < records; ++record) {
                bool removed = false;
                for (auto i = remaining.begin(); i != remaining.end(); ++i) {
                    if (nodes[*i].listener == key) {
                        expectedFree.push_back(reinterpret_cast<uintptr_t>(&nodes[*i]));
                        remaining.erase(i);
                        removed = true;
                        break;
                    }
                }
                if (removed) {
                    expectedPacked |= 0xc;
                    for (auto i : remaining) expectedPacked &= (nodes[i].mask << 2) | 0xfffffff3u;
                }
            }
        }
        freedNodes.clear(); boundaries.fill(0); begins = ends = 0;
        reinterpret_cast<void (__thiscall*)(void*, int32_t, uint32_t)>(0x102beef0)(tool.data(), 120, 0);
        require(freedNodes == expectedFree, "filtered cleanup free sequence");
        require(boundaries[0] == 1 && boundaries[1] == expectedFree.size() && !boundaries[2] && !boundaries[3], "cleanup boundaries");
        require(begins == 2 && ends == 2 && object == expectedObject, "cleanup changed viewport state");
        require(at<uint32_t>(cleanupElement+0x40) == expectedPacked, "filtered packed mask");
        const auto expectedAddress = reinterpret_cast<uintptr_t>(expectedElement.data());
        at<uint32_t>(expectedAddress+0x40) = expectedPacked;
        at<Node*>(expectedAddress+0x44) = remaining.empty() ? nullptr : &nodes[remaining[0]];
        require(element == expectedElement, "cleanup changed unrelated element bytes");
        for (unsigned i = 0; i < nodeCount; ++i)
            require(nodes[i].listener == originalNodes[i].listener && nodes[i].mask == originalNodes[i].mask,
                "cleanup changed callback payload");
        auto* head = at<Node*>(cleanupElement+0x44);
        // Unprocessed lists stay intact; processed lists lose only first matches.
        for (auto i : remaining) { require(head == &nodes[i], "remaining callback chain"); head = head->next; }
        require(!head, "unexpected remaining callback tail");
        ++cases;
    }
    observer.configure(false);
    cleanupMode = false;
    return cases;
}

int main(int argc, char** argv) {
    try {
        require(argc == 2, "expected installed IFM path");
        AddVectoredExceptionHandler(0, diagnoseFailure);
        NativePrivateWorldImageFixture image(argv[1], 0xfd43f8);
        image.seal();
        at<uintptr_t>(0x10a7bac0) = reinterpret_cast<uintptr_t>(update);
        at<uintptr_t>(0x10a7e460) = reinterpret_cast<uintptr_t>(beginProfile);
        at<uintptr_t>(0x10a7e48c) = reinterpret_cast<uintptr_t>(endProfile);
        at<uintptr_t>(0x10a7e45c) = 0;
        at<uintptr_t>(0x10fd4398) = 0;
        at<uintptr_t>(0x10fd43f0) = 0;
        at<uintptr_t>(0x10fd43f4) = 0;
        // These entries are verified against RTTI and constructor stores by Python.
        const std::array<uintptr_t, 8> tables = {0x10ab0fbc, 0x10ab8fa0, 0x10ab90a0,
            0x10ac2954, 0x10ac6684, 0x10ac7348, 0x10ac8b94, 0x10ac9f7c};
        const std::array<unsigned, 8> offsets = {0x1c, 0x24, 0x14, 0xc, 0x18, 0x14, 0x38, 0x18};
        unsigned cases = 0;
        std::array<unsigned, 8> coverage{};
        for (unsigned route = 0; route < tables.size(); ++route) {
            viewport = route == 7;
            PrivateX86BoundaryObserverFixture observer{
                viewport ? std::array<uint32_t, 4>{0x101afc40, 0x101b05d0, 0x10278e90, 0x1029aae0}
                         : std::array<uint32_t, 4>{0x1029a950, 0x10002800, 0x10002600, 0x10002700}, boundary};
            observer.configure(true);
            for (int32_t time : {INT32_MIN, -1250, 0, 120, 10000, INT32_MAX})
            for (unsigned flag = 0; flag < 2; ++flag)
            for (unsigned hasDocument = 0; hasDocument < 2; ++hasDocument)
            for (unsigned controllerTarget = 0; controllerTarget < 2; ++controllerTarget) {
                // Ordinary time-helper auto-scroll and its nested listeners are not executed.
                if (route == 3 && !flag) continue;
                alignas(4) std::array<uint8_t, 0x400> object{};
                alignas(4) std::array<uint8_t, 0x400> tool{};
                alignas(4) std::array<uint8_t, 16> doc{};
                std::array<uintptr_t, 0x114 / 4> primary{};
                primary[0x110 / 4] = reinterpret_cast<uintptr_t>(refreshTrack);
                primary[0xcc / 4] = reinterpret_cast<uintptr_t>(positionOverlay);
                base = reinterpret_cast<uintptr_t>(object.data());
                document = hasDocument ? reinterpret_cast<uintptr_t>(doc.data()) : 0;
                target = base;
                at<uintptr_t>(base) = reinterpret_cast<uintptr_t>(primary.data());
                const auto listener = base + offsets[route];
                at<uintptr_t>(listener) = tables[route];
                if (route == 1) { target = controllerTarget ? base + 0x180 : 0; at<uintptr_t>(listener + 8) = target; }
                if (route == 2) { target = base + 0x180; at<uintptr_t>(listener + 0x10) = target; }
                const auto owner = reinterpret_cast<uintptr_t>(tool.data());
                const uintptr_t list[] = {listener};
                at<uintptr_t>(owner + 0x344) = document;
                at<uintptr_t>(owner + 0x370) = reinterpret_cast<uintptr_t>(list);
                at<int32_t>(owner + 0x37c) = 1;
                at<uintptr_t>(0x10f32568) = owner;
                const auto before = object;
                suppliedTime = time ^ 0x13579;
                updates = virtualUpdates = begins = ends = 0;
                boundaries.fill(0);
                reinterpret_cast<void (__thiscall*)(void*, int32_t, uint32_t)>(0x102beef0)(tool.data(), time, flag);
                require(begins == ends && begins == (route >= 3 && route != 6 ? 2u : 1u), "profile count");
                require(updates == (route == 0 || route == 2 || route == 5 || route == 6 || (route == 1 && controllerTarget) ? 1u : 0u), "update count");
                require(virtualUpdates == (route == 4 || (route == 5 && !flag) ? 1u : 0u), "virtual refresh count");
                // A null root clip exits before the active-clip lookup.
                if (viewport) require(boundaries == std::array<unsigned, 4>{1, 1, hasDocument, 0}, "viewport boundary counts");
                else require(boundaries == std::array<unsigned, 4>{hasDocument && (route == 3 || (route == 5 && !flag)) ? 1u : 0u, 0, 0, 0}, "time getter count");
                auto expected = before;
                if (route == 3) std::memcpy(expected.data() + offsets[route] + 0x24, hasDocument ? &suppliedTime : &time, sizeof(time));
                require(object == expected, "listener unexpectedly modified object");
                ++coverage[route]; ++cases;
            }
            observer.configure(false);
        }
        const auto cleanupCases = runTrackedCleanup();
        const auto subscriptionCases = runSubscriptions();
        image.verify();
        std::cout << "{\"cases\":" << cases << ",\"coverage\":[";
        for (unsigned i = 0; i < coverage.size(); ++i) std::cout << (i ? "," : "") << coverage[i];
        std::cout << "],\"cleanup_cases\":" << cleanupCases << ",\"subscription_cases\":" << subscriptionCases << ",\"image_code_unchanged\":true}\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
