#include "source_map_simulation_clock_candidate.h"

#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

struct Query {
    int32_t tick, maxClients, signonState, eventCredits;
    float tickInterval, hostStep, fireTime;
    uint32_t requested, restoring, alternate, steppingEvents;
};

struct Reply {
    int32_t tick;
    float time, frameTime;
    uint32_t dispatched, clientsOnly, physics, simulatingPhysics, eventService, eventDue;
};
static_assert(sizeof(Query) == 44 && sizeof(Reply) == 36);

SourceMapServerFrameCandidate engineInputs(const Query& query) {
    return sourceMapServerFrameCandidate(query.tick, query.tickInterval, query.hostStep,
        query.requested != 0, query.maxClients, query.signonState);
}

Reply engineReply(const SourceMapServerFrameCandidate& frame) {
    Reply reply{};
    reply.tick = frame.tick;
    reply.time = frame.time;
    reply.frameTime = frame.frameTime;
    reply.dispatched = frame.simulating;
    return reply;
}

#ifdef NATIVE_MAP_SIMULATION_CLOCK
#include <windows.h>
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"

template<typename T> T& cell(uintptr_t address) {
    return *reinterpret_cast<T*>(address);
}

namespace {
Reply* currentReply = nullptr;
uint32_t callbackCount = 0;
std::array<uint32_t, 10> serverGlobals{};

void __fastcall observeEngineGameFrame(void*, void*, uint32_t simulating) {
    ++callbackCount;
    currentReply->dispatched = simulating;
    currentReply->tick = reinterpret_cast<int32_t (__cdecl*)()>(0x101e5760)();
    currentReply->time = reinterpret_cast<float (__cdecl*)()>(0x101e5740)();
    currentReply->frameTime = reinterpret_cast<float (__cdecl*)()>(0x101e5750)();
}

uint32_t __fastcall engineToolStatus(void*, void*) { return 0; }

void engineBoundary(unsigned which, CONTEXT& context) {
    if (which != 0) throw std::runtime_error("unexpected engine boundary");
    PrivateX86BoundaryObserverFixture::returnCdecl(context);
}

void serverBoundary(unsigned which, CONTEXT& context) {
    using Observer = PrivateX86BoundaryObserverFixture;
    if (which == 0) {
        Observer::returnCdecl(context);
        context.Eip = 0x1014311b;
    } else if (which == 1) {
        currentReply->physics = 1;
        currentReply->simulatingPhysics = cell<uint32_t>(context.Esp + 4);
        Observer::returnCdecl(context);
        context.Esp += 4;
        context.Eip = 0x1014312c;
    } else if (which == 2) {
        currentReply->eventService = 1;
        Observer::returnCdecl(context);
        context.Eip = 0x101431ba;
    } else {
        currentReply->clientsOnly = 1;
        Observer::returnCdecl(context);
    }
}

void eventBoundary(unsigned which, CONTEXT& context) {
    if (which != 0) throw std::runtime_error("unexpected queue boundary");
    currentReply->eventDue = 1;
    context.Eip = 0x100e9ff4;
}

void writeGlobals(const SourceMapServerFrameCandidate& frame, int32_t maxClients) {
    std::memcpy(&serverGlobals[3], &frame.time, sizeof(float));
    std::memcpy(&serverGlobals[4], &frame.frameTime, sizeof(float));
    serverGlobals[5] = static_cast<uint32_t>(maxClients);
    serverGlobals[6] = static_cast<uint32_t>(frame.tick);
}

struct EngineReference {
    std::array<uint32_t, 28> client{};
    std::array<uintptr_t, 5> table{};
    uintptr_t object = reinterpret_cast<uintptr_t>(table.data());
    PrivateX86BoundaryObserverFixture observer{
        {0x100a85b0, 0x1011d340, 0x1011d350, 0x1011d360}, engineBoundary};

    EngineReference() {
        table[4] = reinterpret_cast<uintptr_t>(observeEngineGameFrame);
        cell<uintptr_t>(0x1066ca84) = reinterpret_cast<uintptr_t>(&object);
        cell<uintptr_t>(0x103ffaa4) = reinterpret_cast<uintptr_t>(client.data());
        cell<uint8_t>(0x1060d610) = 0;
        cell<uint32_t>(0x10402614) = 0;
        observer.configure(true);
    }

    Reply run(const Query& query) {
        cell<int32_t>(0x1060d4f4) = query.tick;
        cell<float>(0x1060d5f4) = query.tickInterval;
        cell<float>(0x104083f4) = query.hostStep;
        cell<int32_t>(0x1060d5ec) = query.maxClients;
        client[0x68 / 4] = query.signonState;
        Reply output{};
        currentReply = &output;
        callbackCount = 0;
        reinterpret_cast<void (__cdecl*)(uint32_t)>(0x1011d280)(query.requested);
        if (callbackCount != 1) throw std::runtime_error("missing server GameFrame callback");
        return output;
    }
};

struct ServerReference {
    std::array<uintptr_t, 0x128 / 4> services{};
    uintptr_t serviceObject = reinterpret_cast<uintptr_t>(services.data());
    std::array<uint32_t, 13> settings{};
    PrivateX86BoundaryObserverFixture observer{
        {0x101103a0, 0x101973b0, 0x100ea150, 0x101d80e0}, serverBoundary};

    ServerReference() {
        services[0x124 / 4] = reinterpret_cast<uintptr_t>(engineToolStatus);
        cell<uintptr_t>(0x10635ca8) = reinterpret_cast<uintptr_t>(&serviceObject);
        cell<uintptr_t>(0x10635ca0) = reinterpret_cast<uintptr_t>(serverGlobals.data());
        cell<uintptr_t>(0x10607e8c) = reinterpret_cast<uintptr_t>(settings.data());
        cell<uint8_t>(0x10635ed4) = 1;
        observer.configure(true);
    }

    Reply run(const Query& query) {
        const auto frame = engineInputs(query);
        writeGlobals(frame, query.maxClients);
        cell<uint8_t>(0x10635d3e) = static_cast<uint8_t>(query.restoring);
        settings[0x30 / 4] = query.alternate;
        Reply output = engineReply(frame);
        currentReply = &output;
        reinterpret_cast<void (__thiscall*)(void*, uint32_t)>(0x10143030)(nullptr, frame.simulating);
        std::memcpy(&output.frameTime, &serverGlobals[4], sizeof(float));
        return output;
    }
};

struct EventReference {
    std::array<uint32_t, 13> queue{};
    PrivateX86BoundaryObserverFixture observer{
        {0x100e9b25, 0x100e9b2b, 0x100e9b2d, 0x100e9b30}, eventBoundary};

    EventReference() {
        cell<uintptr_t>(0x10635ca0) = reinterpret_cast<uintptr_t>(serverGlobals.data());
        observer.configure(true);
    }

    Reply run(const Query& query) {
        const auto frame = engineInputs(query);
        writeGlobals(frame, query.maxClients);
        cell<uint8_t>(0x106065d1) = static_cast<uint8_t>(query.steppingEvents);
        cell<int32_t>(0x1059133c) = query.eventCredits;
        queue[0x30 / 4] = reinterpret_cast<uintptr_t>(&query.fireTime);
        Reply output = engineReply(frame);
        currentReply = &output;
        reinterpret_cast<void (__thiscall*)(void*)>(0x100e9ad0)(queue.data());
        return output;
    }
};
}
#endif

template<typename Evaluate>
void compareRows(const char* queryPath, const char* replyPath, Evaluate evaluate) {
    std::ifstream input(queryPath, std::ios::binary);
    std::ofstream output(replyPath, std::ios::binary);
    if (!input || !output) throw std::runtime_error("clock fixture I/O");
    size_t count = 0;
    Query query{};
    while (input.read(reinterpret_cast<char*>(&query), sizeof(query))) {
        const Reply reply = evaluate(query);
        output.write(reinterpret_cast<const char*>(&reply), sizeof(reply));
        ++count;
    }
    if (!input.eof() || input.gcount() != 0 || count == 0 || !output) {
        throw std::runtime_error("clock fixture extent");
    }
    std::cout << "clock queries=" << count << '\n';
}

int main(int argc, char** argv) {
    try {
        if (argc != 5) throw std::runtime_error("usage: map-clock engine|server|event reference-or-dash queries replies");
        const std::string mode = argv[1];
        if (mode != "engine" && mode != "server" && mode != "event") {
            throw std::runtime_error("unknown clock comparison");
        }
#ifdef NATIVE_MAP_SIMULATION_CLOCK
        NativePrivateWorldImageFixture image(argv[2]);
        image.seal();
        if (mode == "engine") {
            EngineReference reference;
            compareRows(argv[3], argv[4], [&](const Query& query) { return reference.run(query); });
        } else if (mode == "server") {
            ServerReference reference;
            compareRows(argv[3], argv[4], [&](const Query& query) { return reference.run(query); });
        } else {
            EventReference reference;
            compareRows(argv[3], argv[4], [&](const Query& query) { return reference.run(query); });
        }
        image.verify();
#else
        compareRows(argv[3], argv[4], [&](const Query& query) {
            const auto frame = engineInputs(query);
            Reply output = engineReply(frame);
            if (mode == "server") {
                const auto plan = sourceMapGameFrameCandidate(frame, query.restoring != 0,
                    query.maxClients, query.alternate != 0);
                output.frameTime = plan.frameTime;
                output.clientsOnly = plan.updateClientsOnly;
                output.physics = plan.runPhysics;
                output.simulatingPhysics = plan.simulatingPhysics;
                output.eventService = plan.serviceEvents;
            } else if (mode == "event") {
                output.eventDue = sourceMapEventDueCandidate(frame.time, query.fireTime,
                    query.steppingEvents != 0, query.eventCredits);
            }
            return output;
        });
#endif
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
