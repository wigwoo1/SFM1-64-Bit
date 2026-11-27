#include "source_map_simulation_clock_candidate.h"

#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
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

struct BudgetQuery {
    float remainder, hostDuration, tickInterval;
    int32_t serverTick;
    uint32_t playingDemo, demoSeeking;
    int32_t demoCursor;
    uint32_t demoPaused, hltv, alternate, continueFrameSequence;
};
static_assert(sizeof(BudgetQuery) == 44 && sizeof(SourceMapHostTickBudget) == 8);

struct DispatchQuery {
    int32_t initialTick, serverState, maxClients, signon, pendingSteps, clientSlots;
    float interval;
    uint32_t loading, panelVisible, consolePresent, consoleVisible,
        reporterPresent, reporterVisible, reporterPending, noclipDuringPause, gameBlocksTick;
};
struct DispatchReply { int32_t tick; uint32_t calls, simulating; float frameTime; };
static_assert(sizeof(DispatchQuery) == 64 && sizeof(DispatchReply) == 16);

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

const DispatchQuery* activeDispatch = nullptr;
DispatchReply* dispatchReply = nullptr;
uint32_t __fastcall panelVisible(void*, void*) { return activeDispatch->panelVisible; }
uint32_t __fastcall consoleVisible(void*, void*) { return activeDispatch->consoleVisible; }
uint32_t __fastcall reporterVisible(void*, void*) { return activeDispatch->reporterVisible; }
uint32_t __fastcall gameBlocksTick(void*, void*) { return activeDispatch->gameBlocksTick; }
void __fastcall noServerNetwork(void*, void*, uint32_t, void*) {}
void __fastcall observeDispatchFrame(void*, void*, uint32_t simulating) {
    ++dispatchReply->calls;
    dispatchReply->simulating = static_cast<uint8_t>(simulating) != 0;
    dispatchReply->frameTime = cell<float>(0x104024d8);
}

void dispatchBoundary(unsigned slot, CONTEXT& context) {
    if (slot < 2) {
        PrivateX86BoundaryObserverFixture::returnCdecl(context);
    } else if (slot == 2) {
        context.Ebx = cell<DWORD>(context.Esp + 4);
        context.Esi = cell<DWORD>(context.Esp);
        context.Eip = 0x101207e0;
    } else {
        throw std::runtime_error("unexpected server dispatch boundary");
    }
}

struct ServerDispatchReference {
    std::array<uintptr_t, 0x80 / 4> game{};
    std::array<uintptr_t, 0x9c / 4 + 1> panel{}, reporter{};
    std::array<uintptr_t, 0x58 / 4 + 1> network{};
    std::array<uintptr_t, 0x14 / 4 + 1> console{};
    std::array<uint32_t, 0x158 / 4 + 1> client{};
    std::array<uint32_t, 0x880 / 4> reporterObject{};
    std::array<uint32_t, 13> noclipValue{}, noclipVar{}, stepVar{};
    std::array<uint32_t, 0x20 / 4> channels{};
    uintptr_t panelObject = reinterpret_cast<uintptr_t>(panel.data());
    uintptr_t gameObject = reinterpret_cast<uintptr_t>(game.data());
    uintptr_t networkObject = reinterpret_cast<uintptr_t>(network.data());
    uintptr_t consoleObject = reinterpret_cast<uintptr_t>(console.data());
    PrivateX86BoundaryObserverFixture observer{
        {0x10131390, 0x100a85b0, 0x10120799, 0x10002800}, dispatchBoundary};

    ServerDispatchReference() {
        game[0x10 / 4] = reinterpret_cast<uintptr_t>(observeDispatchFrame);
        game[0x60 / 4] = reinterpret_cast<uintptr_t>(gameBlocksTick);
        panel[0x9c / 4] = reinterpret_cast<uintptr_t>(panelVisible);
        reporter[0x9c / 4] = reinterpret_cast<uintptr_t>(reporterVisible);
        console[0x14 / 4] = reinterpret_cast<uintptr_t>(consoleVisible);
        network[0x58 / 4] = reinterpret_cast<uintptr_t>(noServerNetwork);
        reporterObject[0] = reinterpret_cast<uintptr_t>(reporter.data());
        noclipVar[0x1c / 4] = reinterpret_cast<uintptr_t>(noclipValue.data());
        cell<uintptr_t>(0x1066ca84) = reinterpret_cast<uintptr_t>(&gameObject);
        cell<uintptr_t>(0x103ffaa4) = reinterpret_cast<uintptr_t>(client.data());
        cell<uintptr_t>(0x1040aacc) = reinterpret_cast<uintptr_t>(channels.data());
        cell<uintptr_t>(0x106455b4) = reinterpret_cast<uintptr_t>(stepVar.data());
        cell<uintptr_t>(0x1066ca78) = reinterpret_cast<uintptr_t>(noclipVar.data());
        cell<uintptr_t>(0x106941c4) = reinterpret_cast<uintptr_t>(&networkObject);
        cell<uintptr_t>(0x1040c988) = 0x1036a3f4;
        cell<uintptr_t>(0x1040c988 + 0x68) = reinterpret_cast<uintptr_t>(&panelObject);
        cell<uint8_t>(0x1060d610) = 0;
        cell<uint32_t>(0x10402614) = 0;
        observer.configure(true);
    }

    DispatchReply run(const DispatchQuery& query) {
        activeDispatch = &query;
        client[0x68 / 4] = query.signon;
        client[(0x150 + 8) / 4] = query.clientSlots;
        cell<uintptr_t>(0x10628c80) = query.reporterPresent ? reinterpret_cast<uintptr_t>(reporterObject.data()) : 0;
        reporterObject[0x87c / 4] = query.reporterPending;
        cell<uintptr_t>(0x106704fc) = query.consolePresent ? reinterpret_cast<uintptr_t>(&consoleObject) : 0;
        cell<uint8_t>(0x105cc8ac) = static_cast<uint8_t>(query.loading);
        noclipValue[0x30 / 4] = query.noclipDuringPause;
        cell<int32_t>(0x1060d4ec) = query.serverState;
        cell<int32_t>(0x1060d4f4) = query.initialTick;
        cell<int32_t>(0x1060d5ec) = query.maxClients;
        cell<int32_t>(0x1060d608) = query.pendingSteps;
        cell<float>(0x1060d5f4) = query.interval;
        cell<float>(0x104083f4) = query.interval;
        DispatchReply reply{};
        dispatchReply = &reply;
        reinterpret_cast<void (__cdecl*)(uint32_t)>(0x10120670)(0);
        reply.tick = cell<int32_t>(0x1060d4f4);
        dispatchReply = nullptr;
        activeDispatch = nullptr;
        return reply;
    }
};

// Adapt the isolated arithmetic span's original stack/register convention.
// The observer returns before the enclosing engine frame calls other services.
__declspec(naked) int32_t invokeHostBudget() {
    __asm {
        push ebp
        mov ebp, esp
        sub esp, 8
        push ebx
        mov eax, 10182407h
        jmp eax
    }
}

__declspec(naked) void returnHostBudget() {
    __asm {
        mov eax, ebx
        pop ebx
        mov esp, ebp
        pop ebp
        ret
    }
}

void hostBudgetBoundary(unsigned slot, CONTEXT& context) {
    if (slot != 0 || cell<uint32_t>(context.Ebp - 8) != context.Ebx)
        throw std::runtime_error("unexpected host tick exit");
    context.Eip = reinterpret_cast<uintptr_t>(returnHostBudget);
}

struct HostBudgetReference {
    alignas(uintptr_t) std::array<uint8_t, 0x708> demo{};
    alignas(uintptr_t) std::array<uint8_t, 0x8840> hltv{};
    std::array<int32_t, 13> alternateVar{};
    PrivateX86BoundaryObserverFixture observer{
        {0x1018248e, 0x10002600, 0x10002700, 0x10002800}, hostBudgetBoundary};

    HostBudgetReference() {
        cell<uintptr_t>(reinterpret_cast<uintptr_t>(demo.data())) = 0x10327104;
        cell<uintptr_t>(reinterpret_cast<uintptr_t>(hltv.data())) = 0x10356a84;
        cell<uintptr_t>(0x106285cc) = reinterpret_cast<uintptr_t>(alternateVar.data());
        observer.configure(true);
    }

    SourceMapHostTickBudget run(const BudgetQuery& query) {
        const auto demoBase = reinterpret_cast<uintptr_t>(demo.data());
        const auto hltvBase = reinterpret_cast<uintptr_t>(hltv.data());
        cell<uint8_t>(demoBase + 0x598) = static_cast<uint8_t>(query.playingDemo);
        cell<uint8_t>(demoBase + 0x599) = static_cast<uint8_t>(query.demoPaused);
        cell<int32_t>(demoBase + 0x5a4) = query.demoSeeking ? 120 : -1;
        cell<int32_t>(demoBase + 0x704) = query.demoCursor;
        cell<uint8_t>(hltvBase + 0x8839) = static_cast<uint8_t>(query.demoPaused);
        cell<uintptr_t>(0x104b60c0) = query.hltv ? hltvBase : demoBase;
        alternateVar[0x30 / 4] = query.alternate != 0;
        cell<float>(0x106460f4) = query.remainder;
        cell<float>(0x10644ea4) = query.hostDuration;
        cell<float>(0x104083f4) = query.tickInterval;
        cell<int32_t>(0x104024e0) = query.serverTick;
        const auto count = invokeHostBudget();
        return {count, cell<float>(0x106460f4)};
    }
};

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

template<typename Input = Query, typename Evaluate>
void compareRows(const char* queryPath, const char* replyPath, Evaluate evaluate) {
    std::ifstream input(queryPath, std::ios::binary);
    std::ofstream output(replyPath, std::ios::binary);
    if (!input || !output) throw std::runtime_error("clock fixture I/O");
    size_t count = 0;
    Input query{};
    while (input.read(reinterpret_cast<char*>(&query), sizeof(query))) {
        const auto reply = evaluate(query);
        output.write(reinterpret_cast<const char*>(&reply), sizeof(reply));
        ++count;
    }
    if (!input.eof() || input.gcount() != 0 || count == 0 || !output) {
        throw std::runtime_error("clock fixture extent");
    }
    std::cout << "clock queries=" << count << '\n';
}

template<class Evaluate>
void compareBudgets(const char* queryPath, const char* replyPath, Evaluate evaluate) {
    SourceMapHostTickBudget previous{};
    int32_t nextTick = 0;
    bool started = false;
    compareRows<BudgetQuery>(queryPath, replyPath, [&](BudgetQuery query) {
        if (query.continueFrameSequence) {
            if (!started) throw std::runtime_error("host tick sequence has no starting state");
            query.remainder = previous.remainder;
            query.serverTick = nextTick;
        }
        previous = evaluate(query);
        nextTick = query.serverTick + previous.ticks;
        started = true;
        return previous;
    });
}

int main(int argc, char** argv) {
    try {
        if (argc != 5) throw std::runtime_error("usage: map-clock engine|server|event|budget|dispatch reference-or-dash queries replies");
        const std::string mode = argv[1];
        if (mode != "engine" && mode != "server" && mode != "event" && mode != "budget" && mode != "dispatch") {
            throw std::runtime_error("unknown clock comparison");
        }
#ifdef NATIVE_MAP_SIMULATION_CLOCK
        NativePrivateWorldImageFixture image(argv[2], mode == "dispatch" ? 0x6941c8 :
            mode == "budget" ? 0x6460f8 : 0x638000);
        image.seal();
        if (mode == "budget") {
            HostBudgetReference reference;
            compareBudgets(argv[3], argv[4], [&](const BudgetQuery& query) { return reference.run(query); });
        } else if (mode == "dispatch") {
            ServerDispatchReference reference;
            compareRows<DispatchQuery>(argv[3], argv[4], [&](const auto& query) { return reference.run(query); });
        } else if (mode == "engine") {
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
        if (mode == "dispatch") {
            compareRows<DispatchQuery>(argv[3], argv[4], [](const auto& query) {
                const bool console = query.panelVisible && query.consolePresent && query.consoleVisible;
                const bool reporter = query.reporterPresent && (query.reporterVisible || query.reporterPending) &&
                    query.clientSlots == 1;
                const bool allows = sourceMapServerAllowsStep(query.serverState, query.maxClients,
                    query.signon, query.loading != 0, console, reporter);
                const auto dispatch = sourceMapServerDispatch(query.serverState, query.maxClients,
                    query.pendingSteps, allows, query.noclipDuringPause != 0, query.gameBlocksTick != 0);
                const int32_t tick = query.initialTick + dispatch.advanceTick;
                if (!dispatch.callGameFrame) return DispatchReply{tick, 0, 0, 0.f};
                const auto frame = sourceMapServerFrameCandidate(tick, query.interval, query.interval,
                    dispatch.requestSimulation, query.maxClients, query.signon);
                return DispatchReply{tick, 1, frame.simulating, frame.frameTime};
            });
            return 0;
        }
        if (mode == "budget") {
            compareBudgets(argv[3], argv[4], [](const BudgetQuery& query) {
                const bool held = query.hltv ? query.demoPaused != 0 :
                    sourceMapRecordedDemoHoldsElapsed(query.playingDemo != 0, query.demoSeeking != 0,
                        query.demoCursor, query.demoPaused != 0);
                return sourceMapHostTickBudget(query.remainder, query.hostDuration, query.tickInterval,
                    query.serverTick, !held, query.alternate != 0);
            });
            return 0;
        }
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
