#include "source_client_collision_partition_candidate.h"

#include <algorithm>
#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

#if defined(NATIVE_CLIENT_COLLISION_PARTITION)
#include <windows.h>
#include "native_private_world_image_fixture.h"
#else
#include "source_spatial_point_partition_candidate.h"
#endif

struct Query {
    uint32_t entityFlags, handle, solidFlags, solidType, surroundingBoundsType, triggerBloat, extraList;
    int32_t modelIndex, modelKind;
    float radius;
    std::array<float, 3> origin, angles;
    std::array<float, 6> collisionBounds, specifiedBounds, cachedRelativeBounds;
};

struct Reply {
    uint32_t entityFlags, membershipCalls, membershipHandle, removeMask, insertMask, moveCalls, moveHandle;
    int32_t modelIndex;
    std::array<float, 3> origin, angles;
    std::array<float, 6> moveBounds, relativeBounds;
};

static_assert(sizeof(Query) == 136 && sizeof(Reply) == 104, "fixture wire size");

#if defined(NATIVE_CLIENT_COLLISION_PARTITION)
namespace {
struct NativeServices {
    std::array<uintptr_t, 8> partitionVtable{};
    std::array<uintptr_t, 10> modelInfoVtable{};
    const uintptr_t* partition = partitionVtable.data();
    const uintptr_t* modelInfo = modelInfoVtable.data();
    std::array<uint32_t, 13> clientSettings{};
    int32_t modelType = 0;
    Reply* reply = nullptr;
};

NativeServices services;

void __fastcall updateLists(void*, void*, uint32_t removeMask, uint32_t insertMask, uint16_t handle) {
    ++services.reply->membershipCalls;
    services.reply->membershipHandle = handle;
    services.reply->removeMask = removeMask;
    services.reply->insertMask = insertMask;
}

void __fastcall moved(void*, void*, uint16_t handle, const float* mins, const float* maxs) {
    ++services.reply->moveCalls;
    services.reply->moveHandle = handle;
    std::copy_n(mins, 3, services.reply->moveBounds.begin());
    std::copy_n(maxs, 3, services.reply->moveBounds.begin() + 3);
}

int32_t __fastcall modelType(void*, void*, const void* model) {
    if (model != &services.modelType) throw std::runtime_error("unexpected model-info input");
    return services.modelType;
}

template<class T>
void writeValue(uint8_t* object, size_t offset, const T& value) {
    std::memcpy(object + offset, &value, sizeof(value));
}

template<class T>
T readValue(const uint8_t* object, size_t offset) {
    T value{};
    std::memcpy(&value, object + offset, sizeof(value));
    return value;
}

template<class Function>
Function method(const void* object, size_t offset) {
    const auto* vtable = readValue<const uint8_t*>(static_cast<const uint8_t*>(object), 0);
    return reinterpret_cast<Function>(readValue<uintptr_t>(vtable, offset));
}

void configureClient() {
    services.partitionVtable[0x18 / 4] = reinterpret_cast<uintptr_t>(&updateLists);
    services.partitionVtable[0x1c / 4] = reinterpret_cast<uintptr_t>(&moved);
    services.modelInfoVtable[0x24 / 4] = reinterpret_cast<uintptr_t>(&modelType);
    *reinterpret_cast<uintptr_t*>(0x106c28d4) = reinterpret_cast<uintptr_t>(&services.partition);
    *reinterpret_cast<uintptr_t*>(0x106c28c8) = reinterpret_cast<uintptr_t>(&services.modelInfo);
    *reinterpret_cast<uintptr_t*>(0x106f6254) = reinterpret_cast<uintptr_t>(services.clientSettings.data());
    // Absolute transforms are resolved before the partition and collideable read them.
    *reinterpret_cast<uint8_t*>(0x10638413) = 1;
}

Reply evaluate(const Query& input) {
    Reply out{};
    services.reply = &out;
    services.modelType = input.modelKind;
    alignas(16) std::array<uint8_t, 0x800> entity{};
    auto* const owner = entity.data();
    auto* const propertyStorage = owner + 0x320;
    writeValue(owner, 0, uintptr_t{0x104c6b94});
    writeValue(owner, 0x60, input.modelKind == -1 ? nullptr : &services.modelType);
    writeValue(owner, 0x94, input.origin);
    writeValue(owner, 0xb8, input.angles);
    writeValue(owner, 0xd8, input.entityFlags);
    writeValue(owner, 0x244, static_cast<int16_t>(input.modelIndex));
    writeValue(owner, 0x6be, static_cast<uint8_t>(input.extraList));
    writeValue(propertyStorage, 0, uintptr_t{0x104eb984});
    writeValue(propertyStorage, 4, owner);
    writeValue(propertyStorage, 8, input.collisionBounds);
    writeValue(propertyStorage, 0x20, static_cast<uint16_t>(input.solidFlags));
    writeValue(propertyStorage, 0x22, static_cast<uint8_t>(input.solidType));
    writeValue(propertyStorage, 0x23, static_cast<uint8_t>(input.triggerBloat));
    writeValue(propertyStorage, 0x24, input.radius);
    writeValue(propertyStorage, 0x28, static_cast<uint16_t>(input.handle));
    writeValue(propertyStorage, 0x2a, static_cast<uint8_t>(input.surroundingBoundsType));
    writeValue(propertyStorage, 0x2c, input.specifiedBounds);
    writeValue(propertyStorage, 0x44, input.cachedRelativeBounds);

    auto* property = method<uint8_t* (__thiscall*)(void*)>(owner, 0x0c)(owner);
    if (property != propertyStorage) throw std::runtime_error("unexpected door collideable");
    out.modelIndex = method<int32_t (__thiscall*)(void*)>(property, 0x18)(property);
    const auto origin = method<const float* (__thiscall*)(void*)>(property, 0x20)(property);
    const auto angles = method<const float* (__thiscall*)(void*)>(property, 0x24)(property);
    std::copy_n(origin, 3, out.origin.begin());
    std::copy_n(angles, 3, out.angles.begin());
    method<void (__thiscall*)(void*)>(owner, 0x168)(owner);
    reinterpret_cast<void (__thiscall*)(void*)>(0x100ce830)(property);
    out.entityFlags = readValue<uint32_t>(owner, 0xd8);
    out.relativeBounds = readValue<std::array<float, 6>>(property, 0x44);
    return out;
}
}
#else
static Reply evaluate(const Query& input) {
    using Partition = SourceClientCollisionPartitionCandidate;
    const SourceClientCollisionPoseCandidate pose{static_cast<int16_t>(input.modelIndex), input.origin, input.angles};
    Partition state;
    state.entityFlags = input.entityFlags;
    state.handle = static_cast<uint16_t>(input.handle);
    state.solidFlags = static_cast<uint16_t>(input.solidFlags);
    state.solidType = static_cast<uint8_t>(input.solidType);
    state.surroundingBoundsType = static_cast<uint8_t>(input.surroundingBoundsType);
    state.triggerBloat = static_cast<uint8_t>(input.triggerBloat);
    state.radius = input.radius;
    state.cachedRelativeBounds = input.cachedRelativeBounds;
    state.collisionBounds = input.collisionBounds;
    state.specifiedBounds = input.specifiedBounds;

    Reply out{};
    out.modelIndex = pose.modelIndex;
    out.origin = pose.origin;
    out.angles = state.collisionAngles(pose);
    const auto kind = static_cast<SourceClientCollisionModelKindCandidate>(input.modelKind);
    const auto lists = sourceClientBasePartitionListsCandidate(state, pose.modelIndex, kind, input.extraList != 0);
    out.membershipCalls = 1;
    out.membershipHandle = lists.handle;
    out.removeMask = lists.removeMask;
    out.insertMask = lists.insertMask;
    if (const auto update = sourceClientPartitionMoveCandidate(state, pose)) {
        out.moveCalls = 1;
        out.moveHandle = update->handle;
        out.moveBounds = update->bounds;
    }
    out.entityFlags = state.entityFlags;
    out.relativeBounds = state.cachedRelativeBounds;
    return out;
}

static void checkOwnedSafety() {
    using Partition = SourceClientCollisionPartitionCandidate;
    Partition state;
    state.entityFlags = Partition::dirtyPartition | Partition::dirtyBounds;
    state.handle = 9;
    state.solidType = 1;
    state.radius = 15.f;
    state.collisionBounds = {-4.f, -2.f, -3.f, 4.f, 2.f, 3.f};
    SourceClientCollisionPoseCandidate pose{13, {20.f, 21.f, 22.f}, {10.f, 20.f, 30.f}};
    bool rejected = false;
    try {
        sourceClientPartitionMoveCandidate(state, pose);
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    if (!rejected || state.entityFlags != (Partition::dirtyPartition | Partition::dirtyBounds) ||
        state.cachedRelativeBounds != std::array<float, 6>{}) {
        throw std::runtime_error("unsupported bounds changed collision state");
    }

    pose.angles = {};
    state.collisionBounds[3] = -5.f;
    rejected = false;
    try {
        sourceClientPartitionMoveCandidate(state, pose);
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    if (!rejected || state.entityFlags != (Partition::dirtyPartition | Partition::dirtyBounds) ||
        state.cachedRelativeBounds != std::array<float, 6>{}) {
        throw std::runtime_error("inverted bounds changed collision state");
    }
    state.collisionBounds[3] = 4.f;
    const auto first = sourceClientPartitionMoveCandidate(state, pose);
    const auto lists = sourceClientBasePartitionListsCandidate(state, pose.modelIndex,
        SourceClientCollisionModelKindCandidate::brush, false);
    if (!first || sourceClientPartitionMoveCandidate(state, pose)) {
        throw std::runtime_error("dirty partition update repeated or missing");
    }
    const SourceSpatialPointPartitionCandidate initial({{9, lists.insertMask, first->bounds}});
    pose.origin[0] += 512.f;
    state.entityFlags |= Partition::dirtyPartition;
    const auto next = sourceClientPartitionMoveCandidate(state, pose);
    if (!next) throw std::runtime_error("translated bounds missing");
    const SourceSpatialPointPartitionCandidate translated({{9, lists.insertMask, next->bounds}});
    if (initial.queryPoint({25.f, 21.f, 22.f}, {}, 4) != std::vector<uint64_t>{9} ||
        !translated.queryPoint({25.f, 21.f, 22.f}, {}, 4).empty() ||
        translated.queryPoint({537.f, 21.f, 22.f}, {}, 4) != std::vector<uint64_t>{9}) {
        throw std::runtime_error("translated partition bounds or client list rejected valid point");
    }
}
#endif

int main(int argc, char** argv) {
    try {
        if (argc != 4) throw std::runtime_error("usage: probe reference-or-dash queries replies");
        std::ifstream input(argv[2], std::ios::binary);
        std::ofstream output(argv[3], std::ios::binary);
        if (!input || !output) throw std::runtime_error("fixture files");
#if defined(NATIVE_CLIENT_COLLISION_PARTITION)
        NativePrivateWorldImageFixture image(argv[1], 0x7cc714);
        configureClient();
        image.seal();
#else
        checkOwnedSafety();
#endif
        Query query{};
        size_t count = 0;
        while (input.read(reinterpret_cast<char*>(&query), sizeof(query))) {
            const Reply result = evaluate(query);
            output.write(reinterpret_cast<const char*>(&result), sizeof(result));
            ++count;
        }
        if (!input.eof() || input.gcount() || !output || !count) {
            throw std::runtime_error("fixture stream extent");
        }
#if defined(NATIVE_CLIENT_COLLISION_PARTITION)
        image.verify();
#endif
        std::cout << "partition queries=" << count << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
