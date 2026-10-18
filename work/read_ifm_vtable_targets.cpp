#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

static uint16_t u16(const std::vector<uint8_t>& b, size_t o) {
    if (o + 2 > b.size()) throw std::runtime_error("truncated PE u16");
    return uint16_t(b[o]) | (uint16_t(b[o + 1]) << 8);
}
static uint32_t u32(const std::vector<uint8_t>& b, size_t o) {
    if (o + 4 > b.size()) throw std::runtime_error("truncated PE u32");
    return uint32_t(b[o]) | (uint32_t(b[o + 1]) << 8) | (uint32_t(b[o + 2]) << 16) | (uint32_t(b[o + 3]) << 24);
}
struct Section { uint32_t rva, rawSize, raw; };

int main(int argc, char** argv) {
    try {
        if (argc < 3) throw std::runtime_error("usage: read_ifm_vtable_targets DLL ADDRESS...");
        std::ifstream f(argv[1], std::ios::binary | std::ios::ate);
        if (!f) throw std::runtime_error("cannot open PE");
        auto size = f.tellg();
        if (size < 0) throw std::runtime_error("cannot size PE");
        std::vector<uint8_t> b(static_cast<size_t>(size));
        f.seekg(0); f.read(reinterpret_cast<char*>(b.data()), size);
        if (!f || b.size() < 64 || b[0] != 'M' || b[1] != 'Z') throw std::runtime_error("invalid PE image");
        uint32_t nt = u32(b, 0x3c);
        if (u32(b, nt) != 0x00004550) throw std::runtime_error("invalid PE signature");
        uint16_t n = u16(b, nt + 6), optionalSize = u16(b, nt + 20);
        size_t opt = nt + 24;
        if (u16(b, opt) != 0x10b) throw std::runtime_error("expected PE32 installed SFM DLL");
        uint32_t base = u32(b, opt + 28);
        size_t sectionTable = opt + optionalSize;
        std::vector<Section> sections;
        for (unsigned i = 0; i < n; ++i) {
            size_t s = sectionTable + size_t(i) * 40;
            sections.push_back({u32(b, s + 12), u32(b, s + 16), u32(b, s + 20)});
        }
        for (int i = 2; i < argc; ++i) {
            uint32_t va = static_cast<uint32_t>(std::stoul(argv[i], nullptr, 0));
            uint32_t rva = va - base;
            bool found = false;
            for (const auto& s : sections) if (rva >= s.rva && rva - s.rva + 4 <= s.rawSize) {
                auto fileOffset = size_t(s.raw) + (rva - s.rva);
                std::cout << "0x" << std::hex << va << " -> 0x" << u32(b, fileOffset) << "\n";
                found = true; break;
            }
            if (!found) throw std::runtime_error("address lacks file-backed bytes");
        }
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << "\n"; return 1; }
}
