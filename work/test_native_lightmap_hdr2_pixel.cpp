#include "native_lightmap_hdr2_pixel_candidate.h"

#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <utility>

namespace {
std::uint32_t readU32(const std::array<std::uint8_t, 4>& b) noexcept {
    return static_cast<std::uint32_t>(b[0]) |
        (static_cast<std::uint32_t>(b[1]) << 8u) |
        (static_cast<std::uint32_t>(b[2]) << 16u) |
        (static_cast<std::uint32_t>(b[3]) << 24u);
}

} // namespace

static_assert(noexcept(nativeLightmapHdr2PixelCandidate(
                  36u, std::declval<const std::array<std::uint32_t, 4>&>(),
                  std::declval<std::array<std::uint8_t, 8>&>())),
              "HDR2 pixel encoding must not throw");

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: test_native_lightmap_hdr2_pixel_x64.exe input.bin output.bin\n";
        return 2;
    }
    std::ifstream input(argv[1], std::ios::binary);
    if (!input) return 3;
    std::ofstream output(argv[2], std::ios::binary | std::ios::trunc);
    if (!output) return 4;

    for (;;) {
        std::array<std::uint8_t, 20> record{};
        input.read(reinterpret_cast<char*>(record.data()), static_cast<std::streamsize>(record.size()));
        const auto count = input.gcount();
        if (count == 0) {
            if (input.eof()) break;
            return 5;
        }
        if (count != static_cast<std::streamsize>(record.size())) return 6;

        const auto format = readU32({record[0], record[1], record[2], record[3]});
        std::array<std::uint32_t, 4> rgba{};
        for (std::size_t channel = 0; channel < rgba.size(); ++channel) {
            const std::size_t offset = 4u + channel * 4u;
            rgba[channel] = readU32({record[offset], record[offset + 1u],
                                     record[offset + 2u], record[offset + 3u]});
        }
        std::array<std::uint8_t, 8> encoded{};
        encoded.fill(0xa5u);
        const bool success = nativeLightmapHdr2PixelCandidate(format, rgba, encoded);
        output.put(static_cast<char>(success ? 1 : 0));
        output.write(reinterpret_cast<const char*>(encoded.data()),
                     static_cast<std::streamsize>(encoded.size()));
        if (!output) return 7;
    }
    if (!input.eof()) return 5;
    output.flush();
    return output ? 0 : 7;
}
