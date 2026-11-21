#include "source_spatial_point_ray_adapter_candidate.h"
#include "spatial_point_partition_test_fixture.h"

#include <fstream>
#include <iostream>

template<class T>
static void get(std::istream& in, T& value) {
    if (!in.read(reinterpret_cast<char*>(&value), sizeof(value))) throw std::runtime_error("fixture truncated");
}

template<class T>
static void put(std::ostream& out, const T& value) {
    out.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

static void writeStored(std::ostream& out, const SourceSpatialPointPartitionCandidate& partition, uint16_t handle) {
    put(out, partition.storedBounds(handle));
    put(out, partition.storedMask(handle));
    put(out, partition.storedFlags(handle));
}

static void mutate(SourceSpatialPointPartitionCandidate& partition, const SpatialFixtureMutation& command,
                   std::ostream& stored) {
    command.validate();
    const auto handle = static_cast<uint16_t>(command.handle);
    switch (command.operation) {
    case SpatialFixtureOperation::none:
        return;
    case SpatialFixtureOperation::move:
        partition.move(handle, command.bounds);
        break;
    case SpatialFixtureOperation::replaceLists:
        partition.replaceLists(handle, command.removeMask, command.insertMask);
        break;
    case SpatialFixtureOperation::destroy:
        partition.destroy(handle);
        return;
    case SpatialFixtureOperation::create:
        if (partition.create({command.entityId, command.insertMask, command.bounds}) != handle) {
            throw std::runtime_error("unexpected reused owned handle");
        }
        break;
    }
    writeStored(stored, partition, handle);
}

int main(int argc, char** argv) {
    try {
        static_assert(sizeof(void*) == 8, "owned x64");
        if (argc != 3 && argc != 4) throw std::runtime_error("fixture output [explicit-swept|dynamic] required");
        const bool explicitSwept = argc == 4;
        const bool dynamic = explicitSwept && std::string(argv[3]) == "dynamic";
        if (explicitSwept && !dynamic && std::string(argv[3]) != "explicit-swept") {
            throw std::runtime_error("unknown fixture mode");
        }
        std::ifstream in(argv[1], std::ios::binary);
        std::ofstream out(argv[2], std::ios::binary), stored(std::string(argv[2])+".stored-boxes", std::ios::binary);
        if (!in || !out || !stored) throw std::runtime_error("fixture streams");
        uint32_t count = 0;
        get(in, count);
        if (count > 4096) throw std::runtime_error("fixture count");
        std::vector<SourceSpatialPointEntryCandidate> entries(count);
        for (auto& entry : entries) {
            get(in, entry.entityId);
            get(in, entry.mask);
            get(in, entry.bounds);
        }
        SourceSpatialPointPartitionCandidate partition(entries);
        put(out, count);
        for (uint16_t handle = 0; handle < count; ++handle) {
            put(out, handle);
            writeStored(stored, partition, handle);
        }

        uint32_t queries = 0;
        get(in, queries);
        put(out, queries);
        for (uint32_t i = 0; i < queries; ++i) {
            if (dynamic) {
                SpatialFixtureMutation command;
                get(in, command);
                mutate(partition, command, stored);
            }
            uint32_t mask, coarse, stop;
            std::array<float, 6> q;
            get(in, mask);
            get(in, q);
            get(in, coarse);
            get(in, stop);
            if (coarse) throw std::runtime_error("coarse point query unsupported");
            std::vector<uint64_t> answer;
            if (explicitSwept) {
                uint32_t swept = 0;
                get(in, swept);
                if (swept > 1) throw std::runtime_error("invalid explicit swept flag");
                const SourceWorldLightRayCandidate ray{{q[0],q[1],q[2]}, {q[3],q[4],q[5]}, swept != 0};
                answer = sourceSpatialPointEnumerateRayCandidate(partition, ray, mask, stop);
            } else {
                answer = partition.queryPoint({q[0],q[1],q[2]}, {q[3],q[4],q[5]}, mask, stop);
            }
            put(out, uint32_t(answer.size()));
            for (uint64_t id : answer) put(out, id);
        }
        out.flush();
        stored.flush();
        if (!out || !stored) throw std::runtime_error("output failed");
        std::cout << "PASS owned x64 point partition entities=" << count << " queries=" << queries << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
