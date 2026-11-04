#include "native_lightmap_pixel_layout_candidate.h"

#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>

namespace {
void writeByte(std::ostream& out, std::uint8_t value) {
    out.put(static_cast<char>(value));
}

void writeU16(std::ostream& out, std::uint16_t value) {
    writeByte(out, static_cast<std::uint8_t>(value));
    writeByte(out, static_cast<std::uint8_t>(value >> 8));
}

void writeU32(std::ostream& out, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32; shift += 8) {
        writeByte(out, static_cast<std::uint8_t>(value >> shift));
    }
}
} // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: test_native_lightmap_pixel_layout_x64.exe input.bin output.bin\n";
        return 2;
    }

    std::ifstream input(argv[1], std::ios::binary);
    if (!input) return 3;
    std::ofstream output(argv[2], std::ios::binary | std::ios::trunc);
    if (!output) return 4;

    for (;;) {
        std::array<unsigned char, 4> bytes{};
        input.read(reinterpret_cast<char*>(bytes.data()), 4);
        const auto count = input.gcount();
        if (count == 0) {
            if (input.eof()) break;
            return 5;
        }
        if (count != 4) return 6;

        const std::uint32_t format = static_cast<std::uint32_t>(bytes[0]) |
            (static_cast<std::uint32_t>(bytes[1]) << 8) |
            (static_cast<std::uint32_t>(bytes[2]) << 16) |
            (static_cast<std::uint32_t>(bytes[3]) << 24);
        NativeLightmapPixelLayoutCandidate descriptor{
            0xDEADBEEFu, 0xA5u, 0x5Au,
            {-32768, 32767, 17, -23},
            {0x11223344u, 0x55667788u, 0x99AABBCCu, 0xDDEEFF00u}};
        const bool success = nativeLightmapPixelLayoutCandidate(format, descriptor);

        writeByte(output, success ? 1u : 0u);
        writeU32(output, descriptor.sourceImageFormat);
        writeByte(output, descriptor.bytesPerPixel);
        writeByte(output, descriptor.flags);
        for (const std::int16_t shift : descriptor.channelShifts) {
            writeU16(output, static_cast<std::uint16_t>(shift));
        }
        for (const std::uint32_t mask : descriptor.channelMasks) writeU32(output, mask);
        if (!output) return 7;
    }

    if (!input.eof()) return 5;
    output.flush();
    return output ? 0 : 7;
}
