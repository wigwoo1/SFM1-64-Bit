#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>

#if defined(NATIVE_SLIDING_DOOR_SPAWN)
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"
#else
#include "source_sliding_door_spawn_candidate.h"
#endif

struct Query {
    std::array<float, 3> origin, moveAngles;
    std::array<float, 6> loadedBounds;
    float lip, speed;
    int32_t spawnPosition;
    uint32_t spawnFlags;
};

struct Reply {
    std::array<float, 3> closedOrigin, openOrigin, initialOrigin, direction;
    float speed;
    uint32_t toggleState, startsLocked, requestedSolidType, requestedMovementType;
};

static_assert(sizeof(Query)==64 && sizeof(Reply)==68, "door fixture record sizes");

#if defined(NATIVE_SLIDING_DOOR_SPAWN)
namespace {
constexpr uintptr_t spawn = 0x101030e0;
constexpr uintptr_t setSolid = 0x100f17b0;
constexpr uintptr_t setOrigin = 0x10261150;
constexpr uintptr_t setMoveType = 0x100c2710;
constexpr uintptr_t stop = 0x1010341e;
constexpr uintptr_t epilogue = 0x101034f7;
constexpr size_t collisionOffset = 0x164;
constexpr size_t localOriginOffset = 0x33c;

std::array<uintptr_t, 0x324 / sizeof(uintptr_t)> virtuals{};
uint8_t* entity = nullptr;
const Query* query = nullptr;
Reply* reply = nullptr;
const char modelName[] = "*1";
unsigned stage = 0;

template<class T>
T read(const void* location, size_t offset) {
    T result{};
    std::memcpy(&result, static_cast<const uint8_t*>(location)+offset, sizeof(result));
    return result;
}

template<class T>
void write(void* location, size_t offset, const T& value) {
    std::memcpy(static_cast<uint8_t*>(location)+offset, &value, sizeof(value));
}

void __fastcall precache(void* self, void*) {
    if (self!=entity || stage!=0) throw std::runtime_error("unexpected door precache");
    stage=1;
}

const char** __fastcall getModelName(void* self, void*, const char** result) {
    if (self!=entity || stage!=2) throw std::runtime_error("unexpected door model request");
    *result=modelName;
    return result;
}

void __fastcall loadModel(void* self, void*, const char* name) {
    if (self!=entity || name!=modelName || stage!=2) throw std::runtime_error("unexpected door model loading");
    write(self,collisionOffset+8,query->loadedBounds);
    stage=3;
}

void observe(unsigned signal, CONTEXT& context) {
    using Observer = PrivateX86BoundaryObserverFixture;
    auto* self = reinterpret_cast<uint8_t*>(context.Ecx);
    const auto* args = reinterpret_cast<const uintptr_t*>(context.Esp);
    if (signal==0) {
        if (self!=entity+collisionOffset || stage!=1 || args[1]!=6) throw std::runtime_error("unexpected collision solid request");
        reply->requestedSolidType=static_cast<uint32_t>(args[1]);
        write(self,0x22,static_cast<uint8_t>(args[1]));
        stage=2;
        Observer::returnCdecl(context);
        context.Esp+=4;
    } else if (signal==1) {
        if (args[1]!=reinterpret_cast<uintptr_t>(entity) || stage!=3 || args[3]!=0 ||
            args[2]!=reinterpret_cast<uintptr_t>(entity+0x3c8)) throw std::runtime_error("unexpected starting origin request");
        const auto position = read<std::array<float,3>>(reinterpret_cast<const void*>(args[2]),0);
        write(entity,localOriginOffset,position);
        stage=4;
        Observer::returnCdecl(context);
    } else if (signal==2) {
        if (self!=entity || (stage!=3 && stage!=4) || args[1]!=7 || args[2]!=0) throw std::runtime_error("unexpected movement type request");
        reply->requestedMovementType=static_cast<uint32_t>(args[1]);
        stage=5;
        Observer::returnCdecl(context);
        context.Esp+=8;
    } else {
        if (context.Esi!=reinterpret_cast<uintptr_t>(entity) || stage!=5) throw std::runtime_error("unexpected door stop point");
        stage=6;
        context.Eip=epilogue;
    }
}

Reply evaluate(const Query& input) {
    std::array<uint8_t,0x500> storage{};
    entity=storage.data();
    query=&input;
    Reply result{};
    reply=&result;
    stage=0;
    write(entity,0,virtuals.data());
    write(entity,0x158,int32_t{-1});
    write(entity,0x118,input.spawnFlags);
    write(entity,0xec,input.speed);
    write(entity,localOriginOffset,input.origin);
    write(entity,0x3b8,input.lip);
    write(entity,0x420,input.moveAngles);
    write(entity,0x45c,input.spawnPosition);
    reinterpret_cast<void(__thiscall*)(void*)>(spawn)(entity);
    if (stage!=6) throw std::runtime_error("incomplete door prefix");
    result.closedOrigin=read<std::array<float,3>>(entity,0x3bc);
    result.openOrigin=read<std::array<float,3>>(entity,0x3c8);
    result.initialOrigin=read<std::array<float,3>>(entity,localOriginOffset);
    result.direction=read<std::array<float,3>>(entity,0x420);
    result.speed=read<float>(entity,0xec);
    result.toggleState=read<uint32_t>(entity,0x3ac);
    result.startsLocked=read<uint8_t>(entity,0x458);
    return result;
}
}
#else
static Reply evaluate(const Query& input) {
    const auto door=sourceSlidingDoorSpawnCandidate(input.origin,input.moveAngles,input.loadedBounds,
                                                    input.lip,input.speed,input.spawnPosition,input.spawnFlags);
    return {door.closedOrigin,door.openOrigin,door.initialOrigin,door.direction,door.speed,
            door.initiallyOpen ? 0u : 1u,door.startsLocked ? 1u : 0u,6,7};
}
#endif

int main(int argc,char** argv) {
    try {
        if (argc!=4) throw std::runtime_error("usage: door reference-or-dash queries replies");
        std::ifstream input(argv[2],std::ios::binary);
        std::ofstream output(argv[3],std::ios::binary);
        if (!input || !output) throw std::runtime_error("door fixture files");
#if defined(NATIVE_SLIDING_DOOR_SPAWN)
        NativePrivateWorldImageFixture image(argv[1],0x6de6c4);
        virtuals[0x1c/4]=reinterpret_cast<uintptr_t>(&getModelName);
        virtuals[0x64/4]=reinterpret_cast<uintptr_t>(&precache);
        virtuals[0x68/4]=reinterpret_cast<uintptr_t>(&loadModel);
        virtuals[0x320/4]=0x102c93c0;
        image.seal();
        PrivateX86BoundaryObserverFixture observer({setSolid,setOrigin,setMoveType,stop},&observe);
        observer.configure(true);
#endif
        size_t count=0;
        Query row{};
        while (input.read(reinterpret_cast<char*>(&row),sizeof(row))) {
            const auto result=evaluate(row);
            output.write(reinterpret_cast<const char*>(&result),sizeof(result));
            ++count;
        }
        if (!input.eof() || input.gcount() || !count || !output) throw std::runtime_error("door fixture extent");
#if defined(NATIVE_SLIDING_DOOR_SPAWN)
        observer.configure(false);
        image.verify();
#endif
        std::cout<<"door queries="<<count<<'\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr<<error.what()<<'\n';
        return 1;
    }
}
