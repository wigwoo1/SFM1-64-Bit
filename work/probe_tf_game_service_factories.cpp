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
    Module(const char* directory, const char* filename) : name(filename) {
        const std::string path = std::string(directory) + "\\" + filename;
        handle = LoadLibraryExA(path.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (!handle) throw std::runtime_error(std::string("LoadLibrary failed: ") + filename + " error=" + std::to_string(GetLastError()));
        const auto base = reinterpret_cast<std::uintptr_t>(handle);
        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) throw std::runtime_error(std::string("invalid DOS header: ") + filename);
        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
            throw std::runtime_error(std::string("module is not x86: ") + filename);
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
    Module(Module&& other) noexcept
        : name(other.name), handle(other.handle), imageSize(other.imageSize), code(std::move(other.code)) { other.handle = nullptr; }
    Module& operator=(Module&&) = delete;
};

int main(int argc, char** argv) {
    try {
        static_assert(sizeof(void*) == 4, "Source 1 game-module provider probe must be x86");
        if (argc != 2) throw std::runtime_error("usage: probe_tf_game_service_factories <copied-bin-dir>");
        if (!SetDllDirectoryA(argv[1]) || !SetCurrentDirectoryA(argv[1])) throw std::runtime_error("failed to isolate DLL search/current directory");
        std::vector<Module> modules;
        for (const char* name : {"tier0.dll", "vstdlib.dll", "steam_api.dll", "client.dll", "gameui.dll", "matchmaking.dll", "server.dll"})
            modules.emplace_back(argv[1], name);
        const char* names[] = {
            "Physics2 Interface v0.3", "Physics2 Interface ActorMgr v0.1", "Physics2 Interface ResourceMgr v0.1",
            "ResourceSystem004", "VMaterialSystem2_001", "RenderDeviceMgr001", "VP4002", "MatSystemSurface006",
            "VDmeMakeFileUtils001", "MeshSystem001", "RenderDevice001", "RenderHardwareConfig001", "SceneSystem_001",
            "WorldRendererMgr001", "RenderSystemSurface001", "PropertyEditorSystem_Source1_001", "MATCHFRAMEWORK_001", "GameUISystemMgr001"
        };
        std::ofstream out("tf-game-service-factories-results.txt", std::ios::binary | std::ios::trunc);
        if (!out) throw std::runtime_error("cannot create result file in isolated copied-bin current directory");
        out << "scope=factory-only scan of four tf game modules; no interface methods, Connect, Init, device creation, or rendering called; cwd and DLL search isolated to copied-bin\n";
        unsigned factoryModules = 0, providers = 0, unresolved = 0;
        for (const auto& module : modules) {
            const auto factory = module.factory();
            if (factory) ++factoryModules;
            out << "module=" << module.name << " loaded=true createinterface=" << (factory ? "true" : "false") << "\n";
        }
        for (const char* name : names) {
            unsigned found = 0;
            for (const auto& module : modules) {
                const auto factory = module.factory();
                if (!factory) continue;
                int status = -1;
                void* object = factory(name, &status);
                if (!object) continue;
                auto** table = *reinterpret_cast<void***>(object);
                const auto address = reinterpret_cast<std::uintptr_t>(table);
                const auto base = reinterpret_cast<std::uintptr_t>(module.handle);
                if (address < base || address >= base + module.imageSize) throw std::runtime_error(std::string("vtable outside provider image: ") + module.name + " / " + name);
                ++found; ++providers;
                out << "interface=" << name << " provider=" << module.name << " status=" << status << " vtable_rva=0x" << std::hex << (address - base) << std::dec << " verified_in_provider_image=true\n";
            }
            if (!found) { ++unresolved; out << "interface=" << name << " provider=none status=1\n"; }
        }
        bool unchanged = true;
        for (const auto& module : modules) unchanged = unchanged && module.unchanged();
        out << "summary=queried_interfaces=" << (sizeof(names) / sizeof(names[0])) << ",factory_modules=" << factoryModules << ",provider_pairs=" << providers << ",unresolved=" << unresolved << ",all_loaded_executable_sections_unchanged=" << (unchanged ? "true" : "false") << "\n";
        out.flush();
        if (!out || !unchanged) throw std::runtime_error("result write failed or a loaded executable section changed");
        std::cout << "PASS factory-only game-module scan; provider pairs=" << providers << ", unresolved=" << unresolved << ", executable code unchanged=true\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return 1;
    }
}
