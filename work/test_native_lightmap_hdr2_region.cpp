#include "native_lightmap_hdr2_region_candidate.h"
#include <fstream>
#include <iostream>

static_assert(sizeof(std::size_t) == 8, "This harness requires x64 size_t");

namespace {
std::uint64_t readField(const std::vector<std::uint8_t>& bytes, std::size_t& cursor, unsigned n) {
    std::uint64_t value = 0;
    for (unsigned i = 0; i < n; ++i) value |= std::uint64_t(bytes[cursor++]) << (8u * i);
    return value;
}
void writeU32(std::ostream& out, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) out.put(static_cast<char>((value >> (8u * i)) & 255u));
}
}

// Input: u32 payload size, then <5 u32, u64, 6 u32>, plane RGBA
// uint32 bit vectors and destination bytes. Output: success u8, length u32,
// destination bytes, input-unchanged u8. No struct padding is serialized.
int main(int argc, char** argv) {
    if (argc != 3) return 2;
    std::ifstream input(argv[1], std::ios::binary);
    if (!input) return 3;
    std::ofstream output(argv[2], std::ios::binary | std::ios::trunc);
    if (!output) return 4;
    for (;;) {
        std::vector<std::uint8_t> length(4);
        input.read(reinterpret_cast<char*>(length.data()), 4);
        if (input.gcount() == 0 && input.eof()) break;
        if (input.gcount() != 4) return 6;
        std::size_t cursor = 0;
        const auto recordBytes = readField(length, cursor, 4);
        if (recordBytes < 52 || recordBytes > 16u * 1024u * 1024u) return 8;
        std::vector<std::uint8_t> record(static_cast<std::size_t>(recordBytes));
        input.read(reinterpret_cast<char*>(record.data()), static_cast<std::streamsize>(record.size()));
        if (input.gcount() != static_cast<std::streamsize>(record.size())) return 6;
        cursor = 0;
        const auto format = static_cast<std::uint32_t>(readField(record, cursor, 4));
        NativeLightmapHdr2RegionCandidate region{};
        region.width = static_cast<std::uint32_t>(readField(record, cursor, 4));
        region.height = static_cast<std::uint32_t>(readField(record, cursor, 4));
        region.x = static_cast<std::uint32_t>(readField(record, cursor, 4));
        region.y = static_cast<std::uint32_t>(readField(record, cursor, 4));
        region.rowStrideBytes = static_cast<std::size_t>(readField(record, cursor, 8));
        const auto mask = readField(record, cursor, 4);
        const auto destBytes = readField(record, cursor, 4);
        std::array<std::size_t, 4> counts{};
        std::uint64_t expected = 52 + destBytes;
        for (auto& count : counts) {
            count = static_cast<std::size_t>(readField(record, cursor, 4));
            if (count > 65536u) return 8;
            expected += count * 16u;
        }
        if (mask > 15 || destBytes > 4u * 1024u * 1024u || expected != recordBytes) return 8;
        std::array<std::vector<NativeLightmapHdr2BitsCandidate>, 4> storage;
        std::array<const std::vector<NativeLightmapHdr2BitsCandidate>*, 4> planes{};
        for (std::size_t k = 0; k < 4; ++k) {
            storage[k].resize(counts[k]);
            for (auto& pixel : storage[k]) for (auto& channel : pixel) {
                channel = static_cast<std::uint32_t>(readField(record, cursor, 4));
            }
            if (mask & (std::uint64_t{1} << k)) planes[k] = &storage[k];
        }
        std::vector<std::uint8_t> destination(static_cast<std::size_t>(destBytes));
        for (auto& value : destination) value = record[cursor++];
        const auto before = storage;
        const bool success = nativeLightmapHdr2RegionCandidate(format, planes, region, destination);
        output.put(static_cast<char>(success ? 1 : 0));
        writeU32(output, static_cast<std::uint32_t>(destination.size()));
        output.write(reinterpret_cast<const char*>(destination.data()), static_cast<std::streamsize>(destination.size()));
        output.put(static_cast<char>(before == storage ? 1 : 0));
        if (!output) return 7;
    }
    if (!input.eof()) return 5;
    output.flush();
    return output ? 0 : 7;
}
