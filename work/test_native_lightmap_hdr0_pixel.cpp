#include "native_lightmap_hdr0_pixel_candidate.h"

#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>

namespace {
void put(std::ostream& out, std::uint8_t value) { out.put(static_cast<char>(value)); }
bool getU32(std::istream& in, std::uint32_t& value, bool& eof) {
    std::array<unsigned char, 4> b{};
    in.read(reinterpret_cast<char*>(b.data()), 4);
    const auto count = in.gcount();
    if (count == 0 && in.eof()) { eof = true; return true; }
    if (count != 4) return false;
    value = static_cast<std::uint32_t>(b[0]) |
        (static_cast<std::uint32_t>(b[1]) << 8) |
        (static_cast<std::uint32_t>(b[2]) << 16) |
        (static_cast<std::uint32_t>(b[3]) << 24);
    return true;
}
}

int main(int argc, char** argv) {
    if (argc != 3) return 2;
    std::ifstream input(argv[1], std::ios::binary);
    if (!input) return 3;
    std::ofstream output(argv[2], std::ios::binary | std::ios::trunc);
    if (!output) return 4;
    for (;;) {
        std::uint32_t format = 0;
        bool eof = false;
        if (!getU32(input, format, eof)) return 6;
        if (eof) break;
        std::array<std::uint32_t, 4> slots{};
        for (auto& slot : slots) {
            bool fieldEof = false;
            if (!getU32(input, slot, fieldEof) || fieldEof) return 6;
        }
        std::array<std::uint8_t, 8> pixel{};
        pixel.fill(0xA5);
        std::uint8_t written = 0x5A;
        const bool success = nativeLightmapHdr0PixelCandidate(format, slots, pixel, written);
        put(output, success ? 1 : 0);
        put(output, written);
        for (const auto byte : pixel) put(output, byte);
        if (!output) return 7;
    }
    output.flush();
    return output ? 0 : 7;
}
