#include "source_map_trigger_io_candidate.h"

#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

using Command = SourceMapTriggerInputCandidate;
using Signal = SourceMapTriggerOutputCandidate;
constexpr uint32_t triggerCount = 4;
constexpr uint32_t actorCount = 6;
constexpr uint32_t entityCount = 2 * triggerCount;

struct Step {
    uint32_t action, trigger, actor;
    int32_t argument;
    float time, delay;
};

struct Reply {
    std::array<uint32_t, triggerCount> contacts{};
    uint32_t pending = 0;
    int32_t credits = 0;
    std::array<uint32_t, 32> delivered{};
};
static_assert(sizeof(Step) == 24 && sizeof(Reply) == 152);

uint32_t delivery(uint32_t target, Command input) {
    return 1 + 4 * target + static_cast<uint32_t>(input);
}

struct Steps {
    enum { start, end, service, remove, respawn, queueTest, disable, queueOpen, queueClose };
};

#ifdef NATIVE_MAP_TRIGGER_IO
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"

namespace {
using Storage = std::array<uint32_t, 0x510 / 4>;
using ActorSlots = std::array<uint32_t, 4>;
using Node = std::array<uint32_t, 0x38 / 4>;

const std::array<const char*, entityCount> names{
    "trigger2", "trigger4", "trigger6", "trigger9",
    "door1", "door3", "door5", "door8",
};
const std::array<const char*, 3> inputNames{"Open", "Close", "TouchTest"};
std::array<Storage, entityCount> objects{};
std::array<Storage, actorCount> actors{};
std::array<ActorSlots, actorCount + entityCount> handles{};
std::array<std::array<uint32_t, actorCount + 8>, triggerCount> touchSlots{};
std::array<uintptr_t, 0x34c / 4> objectTable{};
std::array<uintptr_t, 0x15c / 4> actorTable{};
std::array<uintptr_t, 0x7c / 4> serverTable{};
std::array<Node, 1024> nodes{};
std::array<std::array<uint32_t, 7>, 12> links{};
std::array<float, 12> clockGlobals{};
Reply* observed = nullptr;
size_t delivered = 0;
uintptr_t server = 0;

template<typename T> T& global(uintptr_t address) {
    return *reinterpret_cast<T*>(address);
}

uint32_t __fastcall zero(void*, void*) { return 0; }
void __fastcall noOp(void*, void*) {}
void __cdecl noLog(int, const char*) {}
const uint32_t* __fastcall handle(void* object, void*) {
    return static_cast<const uint32_t*>(object) + 0x84 / 4;
}

uint32_t __fastcall accept(void* object, void*, const char* command, void*, void*,
    std::array<uint32_t, 5>, int32_t) {
    const auto index = std::find_if(objects.begin(), objects.end(),
        [&](const Storage& candidate) { return candidate.data() == object; });
    const auto input = std::find(inputNames.begin(), inputNames.end(), command);
    if (index == objects.end() || input == inputNames.end() || !observed ||
        delivered >= observed->delivered.size()) {
        throw std::runtime_error("unexpected map input dispatch");
    }
    const auto target = static_cast<uint32_t>(index - objects.begin());
    const auto kind = static_cast<Command>(input - inputNames.begin());
    observed->delivered[delivered++] = delivery(target, kind);
    if (kind == Command::touchTest && target < triggerCount) {
        reinterpret_cast<void (__thiscall*)(void*, void*)>(0x10254cd0)(object, nullptr);
    }
    return 1;
}

void boundary(unsigned which, CONTEXT& context) {
    using Observer = PrivateX86BoundaryObserverFixture;
    const auto* args = reinterpret_cast<const uintptr_t*>(context.Esp);
    if (which == 0) {
        const auto name = std::find(names.begin(), names.end(), reinterpret_cast<const char*>(args[2]));
        if (name == names.end()) throw std::runtime_error("unknown fixture target name");
        const uintptr_t result = args[1] ? 0 : reinterpret_cast<uintptr_t>(objects[name-names.begin()].data());
        Observer::returnCdecl(context, result);
        context.Esp += 24;
    } else {
        if (which > 2) throw std::runtime_error("unexpected queue callback");
        if (which == 1) *reinterpret_cast<char*>(args[1]) = '\0';
        Observer::returnCdecl(context);
    }
}

void enqueueOriginal(uint32_t target, Command command, float delay) {
    using Add = void (__thiscall*)(void*, const char*, const char*, std::array<uint32_t, 5>,
        float, void*, void*, int32_t);
    reinterpret_cast<Add>(0x100e9860)(reinterpret_cast<void*>(0x10597d48),
        names.at(target), inputNames.at(static_cast<size_t>(command)), {}, delay, nullptr, nullptr, 0);
}

class Map {
    size_t linkCount_ = 0;

    void link(uint32_t owner, uint32_t offset, uint32_t target, Command input) {
        auto& action = links.at(linkCount_++);
        action = {reinterpret_cast<uintptr_t>(names.at(target)),
            reinterpret_cast<uintptr_t>(inputNames.at(static_cast<size_t>(input))),
            0, 0, 0xffffffff, static_cast<uint32_t>(linkCount_), 0};
        objects.at(owner).at((offset+0x14)/4) = reinterpret_cast<uintptr_t>(action.data());
    }

    void activate(uint32_t actor, bool client) {
        auto& entry = handles.at(actor);
        ++entry[1];
        entry[0] = reinterpret_cast<uintptr_t>(actors.at(actor).data());
        actors.at(actor)[0x84/4] = (entry[1] << 16) | actor;
        actors.at(actor)[0x120/4] = client ? 0x80 : 0;
    }

public:
    Map() {
        handles = {};
        objects = {};
        actors = {};
        global<uintptr_t>(0x105a01e0) = reinterpret_cast<uintptr_t>(handles.data())-4;
        global<uintptr_t>(0x10635ca0) = reinterpret_cast<uintptr_t>(clockGlobals.data());
        global<uintptr_t>(0x106de26c) = reinterpret_cast<uintptr_t>(&server);
        global<uintptr_t>(0x1043c220) = reinterpret_cast<uintptr_t>(noLog);
        serverTable[0x74/4] = reinterpret_cast<uintptr_t>(noOp);
        serverTable[0x78/4] = reinterpret_cast<uintptr_t>(noOp);
        server = reinterpret_cast<uintptr_t>(serverTable.data());
        global<uint32_t>(0x10597d78) = 0;
        for (size_t i = 0; i < nodes.size(); ++i) {
            nodes[i][0] = i+1 == nodes.size() ? 0 : reinterpret_cast<uintptr_t>(nodes[i+1].data());
        }
        global<uint32_t>(0x1060a56c) = 0x38;
        global<uintptr_t>(0x1060a578) = reinterpret_cast<uintptr_t>(nodes.front().data());
        global<uint32_t>(0x1060a57c) = 0;
        objectTable[0x08/4] = reinterpret_cast<uintptr_t>(handle);
        objectTable[0x9c/4] = reinterpret_cast<uintptr_t>(accept);
        for (const auto& [slot, original] : {std::pair{0x330, 0x102561f0},
                 {0x340, 0x102563e0}, {0x344, 0x10254310}, {0x348, 0x10254330}}) {
            objectTable[slot/4] = static_cast<uintptr_t>(original);
        }
        actorTable[0x08/4] = reinterpret_cast<uintptr_t>(handle);
        actorTable[0x158/4] = reinterpret_cast<uintptr_t>(zero);
        for (uint32_t i = 0; i < actorCount; ++i) {
            actors[i][0] = reinterpret_cast<uintptr_t>(actorTable.data());
            activate(i, i+1 != actorCount);
        }
        for (uint32_t i = 0; i < entityCount; ++i) {
            auto& object = objects[i];
            object[0] = reinterpret_cast<uintptr_t>(objectTable.data());
            object[0x84/4] = 0x10000 | (i+actorCount);
            object[0x118/4] = 1;
            object[0x428/4] = UINT32_MAX;
            object[0x5c/4] = reinterpret_cast<uintptr_t>(i < triggerCount ? "trigger_multiple" : "func_door");
            object[0x124/4] = reinterpret_cast<uintptr_t>(names[i]);
            handles[i+actorCount] = {reinterpret_cast<uintptr_t>(object.data()), 1, 0, 0};
        }
        for (uint32_t i = 0; i < triggerCount; ++i) {
            auto& object = objects[i];
            object[0x4bc/4] = reinterpret_cast<uintptr_t>(touchSlots[i].data());
            object[0x4c0/4] = touchSlots[i].size();
            link(i, 0x42c, i+triggerCount, Command::open);
            if (i < 2) {
                link(i, 0x474, i+triggerCount, Command::close);
            } else {
                link(i, 0x45c, i, Command::touchTest);
                link(i, 0x4a4, i+triggerCount, Command::close);
            }
        }
    }

    Reply step(const Step& request) {
        Reply reply;
        observed = &reply;
        delivered = 0;
        clockGlobals[3] = request.time;
        const auto trigger = request.trigger;
        auto* actor = reinterpret_cast<void*>(handles.at(request.actor)[0]);
        switch (request.action) {
        case Steps::start:
        case Steps::end:
            if (actor) {
                uintptr_t function = request.action == Steps::start ? 0x1025c410 : 0x102588f0;
                reinterpret_cast<void (__thiscall*)(void*,void*)>(function)(objects.at(trigger).data(), actor);
            }
            break;
        case Steps::service:
            global<uint8_t>(0x106065d1) = static_cast<uint8_t>(request.trigger != 0);
            global<int32_t>(0x1059133c) = request.argument;
            reinterpret_cast<void (__cdecl*)()>(0x100ea150)();
            reply.credits = global<int32_t>(0x1059133c);
            break;
        case Steps::remove:
            handles.at(request.actor)[0] = 0;
            break;
        case Steps::respawn:
            activate(request.actor, request.argument != 0);
            break;
        case Steps::queueTest:
            enqueueOriginal(trigger, Command::touchTest, request.delay);
            break;
        case Steps::disable:
            objects.at(trigger)[0x420 / 4] = request.argument != 0;
            break;
        case Steps::queueOpen:
        case Steps::queueClose:
            enqueueOriginal(trigger+triggerCount, request.action == Steps::queueOpen ? Command::open : Command::close,
                            request.delay);
            break;
        default: throw std::runtime_error("unknown fixture step");
        }
        for (size_t i = 0; i < triggerCount; ++i) reply.contacts[i] = objects[i][0x4c8/4];
        for (auto item = global<const uint32_t*>(0x10597d78); item; item = reinterpret_cast<const uint32_t*>(item[12])) {
            if (++reply.pending > nodes.size()) throw std::runtime_error("cyclic reference event queue");
        }
        return reply;
    }
};
}
#else
class Map {
    struct Actor {
        uint32_t serial = 1;
        bool live = true;
        bool client = true;
    };
    std::array<Actor, actorCount> actors_{};
    std::array<SourceClientMapTriggerCandidate, triggerCount> triggers_{};
    SourceMapTriggerQueueCandidate queue_;

    void emit(uint32_t trigger, float now, Signal signal) {
        const auto door = trigger + triggerCount;
        if (signal == Signal::startTouch) queue_.enqueue(now, 0.f, door, Command::open);
        if (trigger < 2 && signal == Signal::endTouchAll) queue_.enqueue(now, 0.f, door, Command::close);
        if (trigger >= 2 && signal == Signal::endTouch) queue_.enqueue(now, 0.f, trigger, Command::touchTest);
        if (trigger >= 2 && signal == Signal::notTouching) queue_.enqueue(now, 0.f, door, Command::close);
    }

public:
    Map() { actors_.back().client = false; }

    Reply step(const Step& request) {
        Reply reply;
        auto& actor = actors_.at(request.actor);
        const auto handle = (actor.serial << 16) | request.actor;
        const auto trigger = request.trigger;
        const auto now = request.time;
        auto emitFor = [&](Signal signal) { emit(trigger, now, signal); };
        switch (request.action) {
        case Steps::start:
            if (actor.live) triggers_.at(trigger).startTouch(handle, actor.client, emitFor);
            break;
        case Steps::end:
            if (actor.live) {
                triggers_.at(trigger).endTouch(handle, [&](uint32_t other) {
                    const auto& candidate = actors_.at(other & 0xffff);
                    return candidate.live && candidate.serial == (other >> 16);
                }, emitFor);
            }
            break;
        case Steps::service: {
            size_t index = 0;
            reply.credits = request.argument;
            queue_.service(now, trigger != 0, reply.credits, [&](const SourceMapTriggerEventCandidate& event) {
                const auto target = event.target;
                reply.delivered.at(index++) = delivery(target, event.input);
                if (event.input == Command::touchTest) {
                    triggers_.at(target).touchTest([&](Signal signal) { emit(target, now, signal); });
                }
            });
            break;
        }
        case Steps::remove:
            actor.live = false;
            break;
        case Steps::respawn:
            ++actor.serial;
            actor.live = true;
            actor.client = request.argument != 0;
            break;
        case Steps::queueTest:
            queue_.enqueue(now, request.delay, trigger, Command::touchTest);
            break;
        case Steps::disable:
            triggers_.at(trigger).setDisabled(request.argument != 0);
            break;
        case Steps::queueOpen:
        case Steps::queueClose:
            queue_.enqueue(now, request.delay, trigger+triggerCount,
                request.action == Steps::queueOpen ? Command::open : Command::close);
            break;
        default: throw std::runtime_error("unknown fixture step");
        }
        for (size_t i = 0; i < triggerCount; ++i) reply.contacts[i] = static_cast<uint32_t>(triggers_[i].touchCount());
        reply.pending = static_cast<uint32_t>(queue_.pendingCount());
        return reply;
    }
};
#endif

int main(int argc, char** argv) {
    try {
        if (argc != 4) throw std::runtime_error("usage: trigger-io queries replies reference-or-dash");
        std::ifstream input(argv[1], std::ios::binary);
        std::ofstream output(argv[2], std::ios::binary);
        if (!input || !output) throw std::runtime_error("map trigger fixture I/O");
#ifdef NATIVE_MAP_TRIGGER_IO
        NativePrivateWorldImageFixture image(argv[3], 0x6de6c4);
        image.seal();
        PrivateX86BoundaryObserverFixture observer{
            {0x1010f600, 0x10381c20, 0x10112f30, 0x100ea010}, boundary};
        observer.configure(true);
#endif
        size_t count = 0;
        uint32_t rows;
        while (input.read(reinterpret_cast<char*>(&rows), sizeof(rows))) {
            if (!rows || rows > 100000) throw std::runtime_error("invalid map trigger fixture extent");
            Map map;
            for (uint32_t i = 0; i < rows; ++i) {
                Step step{};
                if (!input.read(reinterpret_cast<char*>(&step), sizeof(step))) {
                    throw std::runtime_error("incomplete map trigger fixture");
                }
                const auto reply = map.step(step);
                output.write(reinterpret_cast<const char*>(&reply), sizeof(reply));
                ++count;
            }
        }
        if (!input.eof() || input.gcount() || !output || !count) throw std::runtime_error("invalid map trigger stream");
#ifdef NATIVE_MAP_TRIGGER_IO
        observer.configure(false);
        image.verify();
#endif
        std::cout << "map trigger steps=" << count << '\n';
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
