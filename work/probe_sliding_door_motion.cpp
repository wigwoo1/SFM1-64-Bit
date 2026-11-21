#include "source_sliding_door_motion_candidate.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

using Point = std::array<float, 3>;

struct Header {
    Point closed, opened;
    float speed;
    uint32_t initiallyOpen, initiallyLocked, steps;
};

struct Step {
    uint32_t action; // 0 physics, 1 open, 2 close, 3 lock, 4 unlock.
    float frameTime;
};

struct Reply {
    Point origin, destination, velocity;
    float localTime, moveDoneTime;
    uint32_t state;
};

static_assert(sizeof(Header) == 40 && sizeof(Step) == 8 && sizeof(Reply) == 48);

#ifdef NATIVE_SLIDING_DOOR_MOTION
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"

namespace {
constexpr size_t localOrigin = 0x33c;
constexpr size_t localVelocity = 0x28c;
constexpr size_t absoluteVelocity = 0x214;
constexpr size_t localTime = 0x270;
constexpr size_t moveDone = 0x278;
constexpr size_t destination = 0x400;
constexpr size_t toggleState = 0x3ac;
constexpr size_t locked = 0x458;
std::array<uint32_t, 0x580 / 4> entity{};
std::array<uintptr_t, 0x324 / 4> virtuals{};
std::array<uint32_t, 12> globals{};
std::array<uint32_t, 32> pushManager{};
std::array<uintptr_t, 2> pushVirtuals{};

template<typename T>
T read(size_t offset) {
    T value{};
    std::memcpy(&value, reinterpret_cast<const uint8_t*>(entity.data()) + offset, sizeof(value));
    return value;
}

template<typename T>
void write(size_t offset, const T& value) {
    std::memcpy(reinterpret_cast<uint8_t*>(entity.data()) + offset, &value, sizeof(value));
}

uint32_t __fastcall returnZero(void*, void*) { return 0; }
uint32_t __fastcall returnTrue(void*, void*, const Point*) { return 1; }
void __fastcall changeField(void*, void*, void*) {}
const int32_t* __fastcall invalidHandle(void*, void*) {
    static const int32_t handle = -1;
    return &handle;
}

void boundary(unsigned index, CONTEXT& context) {
    using Observer = PrivateX86BoundaryObserverFixture;
    // Sound, think registration, and collision bookkeeping are supplied by
    // the isolated fixture. The pusher still calculates and writes its pose.
    const auto* args = reinterpret_cast<const uint32_t*>(context.Esp);
    if (index == 0 && (context.Ecx != reinterpret_cast<uintptr_t>(entity.data()) || args[1] != 1)) {
        throw std::runtime_error("unexpected open sound callback");
    }
    if (index >= 2 && context.Ecx != reinterpret_cast<uintptr_t>(pushManager.data())) {
        throw std::runtime_error("unexpected collision manager callback");
    }
    Observer::returnCdecl(context);
    context.Esp += index == 1 ? 12 : 4;
}

void installBindings() {
    virtuals[0x08 / 4] = reinterpret_cast<uintptr_t>(invalidHandle);
    virtuals[0xc4 / 4] = 0x102302e0;
    virtuals[0xdc / 4] = reinterpret_cast<uintptr_t>(returnZero);
    virtuals[0x158 / 4] = reinterpret_cast<uintptr_t>(returnZero);
    virtuals[0x2ec / 4] = reinterpret_cast<uintptr_t>(changeField);
    virtuals[0x2fc / 4] = reinterpret_cast<uintptr_t>(changeField);
    virtuals[0x320 / 4] = 0x102c93c0;
    pushVirtuals[1] = reinterpret_cast<uintptr_t>(returnTrue);
    *reinterpret_cast<uintptr_t*>(0x10635ca0) = reinterpret_cast<uintptr_t>(globals.data());
    *reinterpret_cast<uintptr_t*>(0x105ee61c) = reinterpret_cast<uintptr_t>(pushManager.data());
}

class Door {
public:
    explicit Door(const Header& header) {
        entity.fill(0);
        pushManager.fill(0);
        pushManager[0] = reinterpret_cast<uintptr_t>(pushVirtuals.data());
        write(0, virtuals.data());
        write(0xec, header.speed);
        // The benchmark's toggle flag stays set. Suppress audio in the isolated server.
        write(0x118, uint32_t{0x1020});
        write(0x11c, uint32_t{0xc00000});
        write(0x156, uint8_t{7});
        write(0x328, int32_t{32});
        for (size_t field : {0x158, 0x15c, 0x25c, 0x3fc}) write(field, int32_t{-1});
        write(localOrigin, header.initiallyOpen ? header.opened : header.closed);
        write(moveDone, -1.f);
        write(toggleState, uint32_t{header.initiallyOpen ? 0u : 1u});
        write(0x3bc, header.closed);
        write(0x3c8, header.opened);
        write(locked, static_cast<uint8_t>(header.initiallyLocked));
    }

    Reply step(const Step& request) {
        write(absoluteVelocity, read<Point>(localVelocity));
        if (request.action == 0) {
            std::memcpy(&globals[4], &request.frameTime, sizeof(float));
            reinterpret_cast<void (__thiscall*)(void*)>(0x10199040)(entity.data());
        } else if (request.action <= 2) {
            const uintptr_t input = request.action == 1 ? 0x101026a0 : 0x10101e60;
            reinterpret_cast<void (__thiscall*)(void*,void*)>(input)(entity.data(), nullptr);
        } else {
            write(locked, static_cast<uint8_t>(request.action == 3));
        }
        return {read<Point>(localOrigin), read<Point>(destination), read<Point>(localVelocity),
            read<float>(localTime), read<float>(moveDone), read<uint32_t>(toggleState)};
    }
};
}
#else
class Door {
    SourceSlidingDoorMotionCandidate state_;
public:
    explicit Door(const Header& header) : state_({header.closed, header.opened,
        header.initiallyOpen ? header.opened : header.closed, {}, header.speed,
        header.initiallyOpen != 0, header.initiallyLocked != 0}) {}

    Reply step(const Step& request) {
        if (request.action == 0) {
            state_.advanceUnobstructed(request.frameTime);
        } else if (request.action <= 2) {
            state_.applyInput(request.action == 1 ? SourceSlidingDoorInputCandidate::open
                                                 : SourceSlidingDoorInputCandidate::close);
        } else {
            state_.setLocked(request.action == 3);
        }
        return {state_.origin(), state_.destination(), state_.velocity(), state_.localTime(),
            state_.moveDoneTime(), static_cast<uint32_t>(state_.state())};
    }
};
#endif

int main(int argc, char** argv) {
    try {
        if (argc != 4) throw std::runtime_error("usage: door-motion queries replies reference-or-dash");
        std::ifstream input(argv[1], std::ios::binary);
        std::ofstream output(argv[2], std::ios::binary);
        if (!input || !output) throw std::runtime_error("door motion fixture I/O");
#ifdef NATIVE_SLIDING_DOOR_MOTION
        NativePrivateWorldImageFixture image(argv[3], 0x6de6c4);
        installBindings();
        image.seal();
        PrivateX86BoundaryObserverFixture observer{
            {0x101010c0, 0x100cec60, 0x10198910, 0x10198790}, boundary};
        observer.configure(true);
#endif
        size_t count = 0;
        Header header{};
        while (input.read(reinterpret_cast<char*>(&header), sizeof(header))) {
            if (!header.steps || header.steps > 100000) throw std::runtime_error("invalid door motion fixture extent");
            Door door(header);
            for (uint32_t index = 0; index < header.steps; ++index) {
                Step request{};
                if (!input.read(reinterpret_cast<char*>(&request), sizeof(request)) || request.action > 4) {
                    throw std::runtime_error("invalid door motion step");
                }
                const auto reply = door.step(request);
                output.write(reinterpret_cast<const char*>(&reply), sizeof(reply));
                ++count;
            }
        }
        if (!input.eof() || input.gcount() || !count || !output) {
            throw std::runtime_error("door motion fixture extent");
        }
#ifdef NATIVE_SLIDING_DOOR_MOTION
        observer.configure(false);
        image.verify();
#endif
        std::cout << "door motion steps=" << count << '\n';
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
