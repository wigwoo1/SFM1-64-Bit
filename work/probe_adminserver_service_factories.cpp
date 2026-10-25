#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using InterfaceFactory = void* (__cdecl*)(const char*, int*);

struct Module {
    const char* name;
    HMODULE handle = nullptr;
    std::uintptr_t imageSize = 0;
    std::vector<std::pair<const std::uint8_t*, std::vector<std::uint8_t>>> code;
    explicit Module(const char* directory, const char* filename) : name(filename) {
        const std::string path = std::string(directory) + "\\" + filename;
        handle = LoadLibraryExA(path.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (!handle) throw std::runtime_error(std::string("LoadLibrary failed: ") + filename + " error=" + std::to_string(GetLastError()));
        const auto base = reinterpret_cast<std::uintptr_t>(handle);
        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE || nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
            throw std::runtime_error(std::string("invalid x86 PE: ") + filename);
        imageSize = nt->OptionalHeader.SizeOfImage;
        for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
            const auto& section = IMAGE_FIRST_SECTION(nt)[i];
            if (!(section.Characteristics & IMAGE_SCN_MEM_EXECUTE) || !section.Misc.VirtualSize) continue;
            const auto* p = reinterpret_cast<const std::uint8_t*>(base + section.VirtualAddress);
            code.push_back({p, std::vector<std::uint8_t>(p, p + section.Misc.VirtualSize)});
        }
    }
    InterfaceFactory factory() const { return reinterpret_cast<InterfaceFactory>(GetProcAddress(handle, "CreateInterface")); }
    bool unchanged() const {
        for (const auto& section : code) if (std::memcmp(section.first, section.second.data(), section.second.size())) return false;
        return true;
    }
    ~Module() { if (handle) FreeLibrary(handle); }
    Module(const Module&) = delete;
    Module& operator=(const Module&) = delete;
};

int main(int argc, char** argv) {
    try {
        static_assert(sizeof(void*) == 4, "Source reference probe must be x86");
        if (argc != 3) throw std::runtime_error("usage: probe_adminserver_service_factories <copied-bin-dir> <output.txt>");
        if (!SetDllDirectoryA(argv[1]) || !SetCurrentDirectoryA(argv[1])) throw std::runtime_error("failed to isolate DLL search/current directory");
        Module tier0(argv[1], "tier0.dll");
        Module vstdlib(argv[1], "vstdlib.dll");
        Module admin(argv[1], "adminserver.dll");
        const char* names[] = {
            "Physics2 Interface v0.3", "Physics2 Interface ActorMgr v0.1", "Physics2 Interface ResourceMgr v0.1",
            "ResourceSystem004", "VMaterialSystem2_001", "RenderDeviceMgr001", "VP4002", "MatSystemSurface006",
            "VDmeMakeFileUtils001", "MeshSystem001", "RenderDevice001", "RenderHardwareConfig001", "SceneSystem_001",
            "WorldRendererMgr001", "RenderSystemSurface001", "PropertyEditorSystem_Source1_001", "MATCHFRAMEWORK_001", "GameUISystemMgr001"
        };
        std::ofstream out(argv[2], std::ios::binary | std::ios::trunc);
        if (!out) throw std::runtime_error("cannot create result file");
        out << "scope=factory-only adminserver candidate; no interface methods, Connect, Init, device creation, or rendering called; cwd and DLL search isolated to copied-bin\n";
        out << "module=tier0.dll loaded=true createinterface=" << (tier0.factory() ? "true" : "false") << "\n";
        out << "module=vstdlib.dll loaded=true createinterface=" << (vstdlib.factory() ? "true" : "false") << "\n";
        out << "module=adminserver.dll loaded=true createinterface=" << (admin.factory() ? "true" : "false") << "\n";
        if (!admin.factory()) throw std::runtime_error("adminserver has no CreateInterface export");
        unsigned found = 0, unresolved = 0;
        for (const char* name : names) {
            int status = -1;
            void* object = admin.factory()(name, &status);
            if (!object) { ++unresolved; out << "interface=" << name << " provider=none status=" << status << "\n"; continue; }
            auto** table = *reinterpret_cast<void***>(object);
            const auto address = reinterpret_cast<std::uintptr_t>(table);
            const auto base = reinterpret_cast<std::uintptr_t>(admin.handle);
            if (address < base || address >= base + admin.imageSize) throw std::runtime_error(std::string("vtable outside adminserver image: ") + name);
            ++found;
            out << "interface=" << name << " provider=adminserver.dll status=" << status << " vtable_rva=0x" << std::hex << (address - base) << std::dec << " verified_in_provider_image=true\n";
        }
        const bool unchanged = tier0.unchanged() && vstdlib.unchanged() && admin.unchanged();
        out << "summary=queried_interfaces=" << (sizeof(names) / sizeof(names[0])) << ",adminserver_providers=" << found << ",unresolved=" << unresolved << ",all_loaded_executable_sections_unchanged=" << (unchanged ? "true" : "false") << "\n";
        out.flush();
        if (!out || !unchanged) throw std::runtime_error("result write failed or a loaded executable section changed");
        std::cout << "PASS factory-only adminserver query; providers=" << found << ", unresolved=" << unresolved << ", executable code unchanged=true\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return 1;
    }
}
