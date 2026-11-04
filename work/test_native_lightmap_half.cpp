#include "native_lightmap_half_candidate.h"

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>

static_assert(noexcept(nativeLightmapHalfBitsCandidate(0u)), "conversion must not throw");
static_assert(nativeLightmapHalfBitsCandidate(0x3f801fffu) == 0x3c00u, "truncate, do not round");
static_assert(nativeLightmapHalfBitsCandidate(0x33800000u) == 1u, "minimum half subnormal");

int wmain(int argc, wchar_t** argv) {
    if (argc != 3) {
        std::cerr << "Usage: test_native_lightmap_half_x64.exe input-u32le.bin output-u16le.bin\n";
        return 2;
    }
    std::ifstream input(std::filesystem::path(argv[1]), std::ios::binary);
    if (!input) {
        std::cerr << "Cannot open input file\n";
        return 3;
    }
    std::ofstream output(std::filesystem::path(argv[2]), std::ios::binary);
    if (!output) {
        std::cerr << "Cannot open output file\n";
        return 4;
    }
    std::array<unsigned char, 4> bytes{};
    while (input.read(reinterpret_cast<char*>(bytes.data()), 4)) {
        const uint32_t bits = static_cast<uint32_t>(bytes[0])
            | (static_cast<uint32_t>(bytes[1]) << 8u)
            | (static_cast<uint32_t>(bytes[2]) << 16u)
            | (static_cast<uint32_t>(bytes[3]) << 24u);
        const uint16_t half = nativeLightmapHalfBitsCandidate(bits);
        const std::array<unsigned char, 2> result{
            static_cast<unsigned char>(half & 0xffu),
            static_cast<unsigned char>(half >> 8u)};
        output.write(reinterpret_cast<const char*>(result.data()), 2);
        if (!output) {
            std::cerr << "Output write failed\n";
            return 5;
        }
    }
    if (!input.eof() || input.gcount() != 0) {
        std::cerr << "Input must contain complete little-endian uint32 values\n";
        return 6;
    }
    output.close();
    if (!output) {
        std::cerr << "Output close failed\n";
        return 7;
    }
    return 0;
}
