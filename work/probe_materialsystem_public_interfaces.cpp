#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

struct ImageSnapshot {
    HMODULE module = nullptr;
    std::uintptr_t base = 0;
    std::size_t imageSize = 0;
    std::vector<std::pair<const std::uint8_t*, std::vector<std::uint8_t>>> executable;

    explicit ImageSnapshot(HMODULE value) : module(value), base(reinterpret_cast<std::uintptr_t>(value)) {
        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE) throw std::runtime_error("invalid DOS image");
        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
            throw std::runtime_error("material system is not an x86 PE image");
        imageSize = nt->OptionalHeader.SizeOfImage;
        for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
            const auto& section = IMAGE_FIRST_SECTION(nt)[i];
            if ((section.Characteristics & IMAGE_SCN_MEM_EXECUTE) == 0 || section.Misc.VirtualSize == 0) continue;
            const auto* begin = reinterpret_cast<const std::uint8_t*>(base + section.VirtualAddress);
            executable.push_back({begin, std::vector<std::uint8_t>(begin, begin + section.Misc.VirtualSize)});
        }
    }

    std::uintptr_t pointerRva(const void* value) const {
        const auto p = reinterpret_cast<std::uintptr_t>(value);
        if (p < base || p - base >= imageSize) return 0;
        return p - base;
    }

    std::string asciiAtRva(std::uintptr_t rva) const {
        if (rva >= imageSize) throw std::runtime_error("string RVA outside material-system image");
        const auto* text = reinterpret_cast<const char*>(base + rva);
        std::size_t size = 0;
        while (size < 256 && rva + size < imageSize && text[size] != '\0') {
            const auto c = static_cast<unsigned char>(text[size]);
            if (c < 0x20 || c > 0x7e) throw std::runtime_error("non-printable dependency string");
            ++size;
        }
        if (size == 256 || rva + size >= imageSize) throw std::runtime_error("unterminated dependency string");
        return std::string(text, size);
    }

    void verify() const {
        for (const auto& section : executable)
            if (std::memcmp(section.first, section.second.data(), section.second.size()) != 0)
                throw std::runtime_error("material-system executable section changed");
    }
};

int main(int argc, char** argv) {
    try {
        static_assert(sizeof(void*) == 4, "This read-only reference probe must be x86");
        if (argc != 4) throw std::runtime_error("usage: probe_materialsystem_public_interfaces <copied-bin-dir> <interface-name> <output.txt>");
        const char* requestedName = argv[2];
        if (std::strcmp(requestedName, "VMaterialSystem080") != 0 &&
            std::strcmp(requestedName, "VMaterialSystem2_001") != 0 &&
            std::strcmp(requestedName, "VMaterialSystemStub001") != 0)
            throw std::runtime_error("interface name is outside the audited safe-query allowlist");
        const std::string dir = argv[1];
        if (!SetDllDirectoryA(dir.c_str())) throw std::runtime_error("SetDllDirectory failed");
        HMODULE tier0 = LoadLibraryExA((dir + "\\tier0.dll").c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        HMODULE vstdlib = LoadLibraryExA((dir + "\\vstdlib.dll").c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        HMODULE filesystem = LoadLibraryExA((dir + "\\filesystem_stdio.dll").c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        HMODULE material = LoadLibraryExA((dir + "\\materialsystem.dll").c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (!tier0 || !vstdlib || !filesystem || !material) throw std::runtime_error("failed to load copied original DLL set");
        ImageSnapshot snapshot(material);
        using Factory = void* (__cdecl*)(const char*, int*);
        const auto factory = reinterpret_cast<Factory>(GetProcAddress(material, "CreateInterface"));
        if (!factory) throw std::runtime_error("materialsystem CreateInterface export missing");

        std::ofstream out(argv[3], std::ios::binary | std::ios::trunc);
        if (!out) throw std::runtime_error("cannot open result file");
        out << "scope=factory-only; no interface methods called; no Connect/Init/Shutdown; copied DLLs only\n";
        out << "materialsystem_image_base=0x" << std::hex << snapshot.base << " image_size=0x" << snapshot.imageSize << std::dec << "\n";
        const char* name = requestedName;
        int status = -999;
        void* object = factory(name, &status);
        out << "interface=" << name << " status=" << status << " object=" << (object ? "present" : "null");
        if (object) {
            void** table = *reinterpret_cast<void***>(object);
            const auto tableRva = snapshot.pointerRva(table);
            out << " vtable_rva=";
            if (tableRva) out << "0x" << std::hex << tableRva << std::dec;
            else out << "external";
            if (tableRva == 0x984cc) {
                out << " app_system_slots_0_to_4=";
                for (unsigned slot = 0; slot < 5; ++slot) {
                    if (slot) out << ',';
                    const auto methodRva = snapshot.pointerRva(table[slot]);
                    if (!methodRva) throw std::runtime_error("app-system method outside material-system image");
                    out << "0x" << std::hex << methodRva << std::dec;
                }
                out << " connect_factory_dependency_names=";
                const std::uintptr_t dependencyRvas[] = {0x9700c, 0x96fec, 0x96fdc, 0x96fcc, 0x96fbc};
                for (unsigned i = 0; i < 5; ++i) {
                    if (i) out << ',';
                    out << snapshot.asciiAtRva(dependencyRvas[i]);
                }
            }
            unsigned inside = 0;
            if (table) {
                for (unsigned slot = 0; slot < 256; ++slot) {
                    void* entry = table[slot];
                    if (!entry) break;
                    const auto rva = snapshot.pointerRva(entry);
                    if (!rva) break;
                    ++inside;
                }
            }
            out << " consecutive_materialsystem_entries=" << inside;
        }
        out << "\n";
        snapshot.verify();
        out << "materialsystem_executable_sections_unchanged=true\n";
        out.flush();
        if (!out) throw std::runtime_error("result write failed");
        std::cout << "PASS copied-DLL public factory query; unknown interface methods were not invoked\n";
        FreeLibrary(material);
        FreeLibrary(filesystem);
        FreeLibrary(vstdlib);
        FreeLibrary(tier0);
        SetDllDirectoryA(nullptr);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
