#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

struct Snapshot {
    std::uintptr_t base = 0;
    std::size_t imageSize = 0;
    std::vector<std::pair<const std::uint8_t*, std::vector<std::uint8_t>>> executable;
    explicit Snapshot(HMODULE module) : base(reinterpret_cast<std::uintptr_t>(module)) {
        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE) throw std::runtime_error("invalid PE DOS header");
        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
            throw std::runtime_error("reference module is not x86");
        imageSize = nt->OptionalHeader.SizeOfImage;
        for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
            const auto& section = IMAGE_FIRST_SECTION(nt)[i];
            if ((section.Characteristics & IMAGE_SCN_MEM_EXECUTE) == 0 || !section.Misc.VirtualSize) continue;
            const auto* p = reinterpret_cast<const std::uint8_t*>(base + section.VirtualAddress);
            executable.push_back({p, std::vector<std::uint8_t>(p, p + section.Misc.VirtualSize)});
        }
    }
    std::uintptr_t rva(const void* p) const {
        const auto value = reinterpret_cast<std::uintptr_t>(p);
        return value >= base && value - base < imageSize ? value - base : 0;
    }
    void verify() const {
        for (const auto& section : executable)
            if (std::memcmp(section.first, section.second.data(), section.second.size()))
                throw std::runtime_error("shader API executable section changed");
    }
};

int main(int argc, char** argv) {
    try {
        static_assert(sizeof(void*) == 4, "reference oracle must be x86");
        if (argc != 4) throw std::runtime_error("usage: probe_shaderapi_public_interfaces <copied-bin-dir> <interface-name> <output.txt>");
        const char* name = argv[2];
        const char* allowed[] = {"ShaderDeviceMgr001", "GShaderDeviceMgr001", "MaterialSystemHardwareConfig013",
                                 "ShaderApi030", "ShaderDevice002", "ShaderShadow010", "GShaderShadow010"};
        bool accepted = false;
        for (const char* value : allowed) if (std::strcmp(name, value) == 0) accepted = true;
        if (!accepted) throw std::runtime_error("interface name outside audited query allowlist");
        const std::string dir = argv[1];
        if (!SetDllDirectoryA(dir.c_str())) throw std::runtime_error("SetDllDirectory failed");
        HMODULE tier0 = LoadLibraryExA((dir + "\\tier0.dll").c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        HMODULE vstdlib = LoadLibraryExA((dir + "\\vstdlib.dll").c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        HMODULE api = LoadLibraryExA((dir + "\\shaderapidx9.dll").c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (!tier0 || !vstdlib || !api) throw std::runtime_error("failed to load copied shader API DLL set");
        Snapshot snapshot(api);
        using Factory = void* (__cdecl*)(const char*, int*);
        auto factory = reinterpret_cast<Factory>(GetProcAddress(api, "CreateInterface"));
        if (!factory) throw std::runtime_error("shaderapidx9 CreateInterface export missing");
        std::ofstream out(argv[3], std::ios::binary | std::ios::trunc);
        if (!out) throw std::runtime_error("cannot create query result");
        out << "scope=factory-only; no interface methods or device initialization called; copied DLLs only\n" << std::flush;
        int status = -999;
        void* object = factory(name, &status);
        out << "module=shaderapidx9.dll interface=" << name << " status=" << status << " object=" << (object ? "present" : "null");
        if (object) {
            void** table = *reinterpret_cast<void***>(object);
            out << " vtable_rva=";
            const auto tableRva = snapshot.rva(table);
            if (tableRva) out << "0x" << std::hex << tableRva << std::dec;
            else out << "external";
            unsigned inside = 0;
            if (table && tableRva) for (unsigned slot = 0; slot < 256; ++slot) {
                const auto entry = table[slot];
                if (!entry || !snapshot.rva(entry)) break;
                ++inside;
            }
            out << " consecutive_shaderapidx9_entries=" << inside;
        }
        out << "\n" << std::flush;
        snapshot.verify();
        out << "shaderapidx9_executable_sections_unchanged=true\n" << std::flush;
        if (!out) throw std::runtime_error("result write failed");
        std::cout << "PASS copied shaderapidx9 factory query; no device initialization or interface method called\n";
        FreeLibrary(api); FreeLibrary(vstdlib); FreeLibrary(tier0); SetDllDirectoryA(nullptr);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
