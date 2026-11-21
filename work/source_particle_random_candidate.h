#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>

using SourceParticleRandomTableCandidate = std::array<float, 4096>;

// Compatibility data from the supported installed SFM revision. Open it as
// a file, copy only the numeric table, and keep it in memory. The editor
// never loads the DLL as executable code and ships without these values.
inline std::optional<SourceParticleRandomTableCandidate> readSourceParticleRandomTableCandidate(
        const std::filesystem::path& reference) {
    std::ifstream file(reference, std::ios::binary);
    std::array<unsigned char, 64> dos{};
    if (!file.read(reinterpret_cast<char*>(dos.data()), dos.size()) || dos[0] != 'M' || dos[1] != 'Z')
        return {};
    uint32_t peOffset{};
    std::memcpy(&peOffset, dos.data() + 60, sizeof(peOffset));
    file.seekg(peOffset);
    std::array<unsigned char, 26> pe{};
    if (!file.read(reinterpret_cast<char*>(pe.data()), pe.size()) || pe[0] != 'P' || pe[1] != 'E' ||
        pe[2] || pe[3] || pe[4] != 0x4c || pe[5] != 1 || pe[24] != 0x0b || pe[25] != 1)
        return {};
    file.seekg(0xeb50c0);
    SourceParticleRandomTableCandidate table{};
    if (!file.read(reinterpret_cast<char*>(table.data()), sizeof(table))) return {};
    uint64_t fingerprint = 14695981039346656037ull;
    const auto* bytes = reinterpret_cast<const unsigned char*>(table.data());
    for (size_t index = 0; index < sizeof(table); ++index)
        fingerprint = (fingerprint ^ bytes[index]) * 1099511628211ull;
    if (fingerprint != 0x4dc312e6fd5b1cc8ull) return {};
    return table;
}

struct SourceParticleRandomCandidate {
    uint32_t seed = 0, consumed = 0;

    uint32_t nextIndex() {
        return (seed + consumed++) & (std::tuple_size_v<SourceParticleRandomTableCandidate> - 1);
    }

    float sample(const SourceParticleRandomTableCandidate& table) {
        return table[nextIndex()];
    }

    float linear(const SourceParticleRandomTableCandidate& table, float minimum, float maximum) {
        return sample(table) * (maximum - minimum) + minimum;
    }

    int32_t sequence(const SourceParticleRandomTableCandidate& table, int32_t minimum, int32_t maximum) {
        return minimum + static_cast<int32_t>(sample(table) * static_cast<float>(maximum - minimum + 1));
    }
};
