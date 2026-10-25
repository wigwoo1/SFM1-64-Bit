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
    std::string name;
    HMODULE handle = nullptr;
    DWORD loadError = ERROR_SUCCESS;
    std::uintptr_t imageSize = 0;
    std::vector<std::pair<const std::uint8_t*, std::vector<std::uint8_t>>> code;

    Module(const char* directory, const char* filename) : name(filename) {
        char fullPath[MAX_PATH]{};
        const auto length = GetFullPathNameA((std::string(directory) + "\\" + filename).c_str(), MAX_PATH, fullPath, nullptr);
        if (!length || length >= MAX_PATH) { loadError = GetLastError(); return; }
        handle = LoadLibraryExA(fullPath, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (!handle) { loadError = GetLastError(); return; }
        const auto base = reinterpret_cast<std::uintptr_t>(handle);
        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE) throw std::runtime_error("invalid PE image: " + name);
        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
            throw std::runtime_error("module is not x86: " + name);
        imageSize = nt->OptionalHeader.SizeOfImage;
        for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
            const auto& section = IMAGE_FIRST_SECTION(nt)[i];
            if (!(section.Characteristics & IMAGE_SCN_MEM_EXECUTE) || !section.Misc.VirtualSize) continue;
            const auto* begin = reinterpret_cast<const std::uint8_t*>(base + section.VirtualAddress);
            code.push_back({begin, std::vector<std::uint8_t>(begin, begin + section.Misc.VirtualSize)});
        }
    }
    InterfaceFactory factory() const {
        return handle ? reinterpret_cast<InterfaceFactory>(GetProcAddress(handle, "CreateInterface")) : nullptr;
    }
    bool unchanged() const {
        for (const auto& section : code)
            if (std::memcmp(section.first, section.second.data(), section.second.size())) return false;
        return true;
    }
    ~Module() { if (handle) FreeLibrary(handle); }
    Module(const Module&) = delete;
    Module& operator=(const Module&) = delete;
    Module(Module&& other) noexcept
        : name(std::move(other.name)), handle(other.handle), loadError(other.loadError), imageSize(other.imageSize), code(std::move(other.code)) {
        other.handle = nullptr;
    }
};

int main(int argc, char** argv) {
    try {
        static_assert(sizeof(void*) == 4, "Source 1 provider oracle must be x86");
        if (argc != 3) throw std::runtime_error("usage: probe_engine_service_provider_factories <copied-bin-dir> <output.txt>");
        if (!SetDllDirectoryA(argv[1])) throw std::runtime_error("SetDllDirectory failed");
        const char* moduleNames[] = {
            "tier0.dll", "vstdlib.dll", "filesystem_stdio.dll", "shaderapidx9.dll", "materialsystem.dll",
            "vphysics.dll", "inputsystem.dll", "datacache.dll", "vgui2.dll", "shaderapidx10.dll", "shaderapiempty.dll",
            "localize.dll", "soundsystem.dll", "soundemittersystem.dll", "studiorender.dll", "scenefilecache.dll",
            "networksystem.dll", "vguimatsurface.dll", "video_services.dll", "valve_avi.dll", "mdllib.dll", "engine.dll",
            "tools\\ifm.dll"
        };
        const char* interfaceNames[] = {
            "Physics2 Interface v0.3", "Physics2 Interface ActorMgr v0.1", "Physics2 Interface ResourceMgr v0.1",
            "ResourceSystem004", "VMaterialSystem2_001", "RenderDeviceMgr001", "SoundSystem001", "VP4002", "VMDLLIB001",
            "VStudioRender026", "Localize_001", "MatSystemSurface006", "VAvi001", "VBik001", "VDmeMakeFileUtils001",
            "VSoundEmitter003", "MeshSystem001", "RenderDevice001", "RenderHardwareConfig001", "SceneSystem_001",
            "WorldRendererMgr001", "RenderSystemSurface001", "PropertyEditorSystem_Source1_001", "IVideoServices002",
            "MATCHFRAMEWORK_001", "GameUISystemMgr001", "NetworkSystemVersion001"
        };
        std::vector<Module> modules;
        for (const char* name : moduleNames) modules.emplace_back(argv[1], name);
        std::ofstream out(argv[2], std::ios::binary | std::ios::trunc);
        if (!out) throw std::runtime_error("cannot create result file");
        out << "scope=factory-only scan of previously unresolved app-system interface names; no interface methods, Connect, Init, device creation, or rendering called\n";
        unsigned loaded = 0, providerPairs = 0, unresolved = 0, factoryModules = 0;
        for (const auto& module : modules) {
            const auto factory = module.factory();
            if (module.handle) ++loaded;
            if (factory) ++factoryModules;
            out << "module=" << module.name << " loaded=" << (module.handle ? "true" : "false")
                << " createinterface=" << (factory ? "true" : "false");
            if (!module.handle) out << " win32_error=" << module.loadError;
            out << "\n";
        }
        for (const char* name : interfaceNames) {
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
                if (address < base || address >= base + module.imageSize)
                    throw std::runtime_error("returned vtable is outside provider image: " + module.name + " / " + name);
                ++found; ++providerPairs;
                out << "interface=" << name << " provider=" << module.name << " status=" << status
                    << " vtable_rva=0x" << std::hex << (address - base) << std::dec << " verified_in_provider_image=true\n";
            }
            if (!found) { ++unresolved; out << "interface=" << name << " provider=none status=1\n"; }
        }
        bool unchanged = true;
        for (const auto& module : modules) unchanged = unchanged && module.unchanged();
        out << "summary=queried_interfaces:" << (sizeof(interfaceNames) / sizeof(interfaceNames[0]))
            << ",provider_pairs:" << providerPairs << ",unresolved_interfaces:" << unresolved
            << ",candidate_modules_loaded:" << loaded << "/" << modules.size() << ",factory_modules:" << factoryModules
            << ",all_candidate_executable_sections_unchanged:" << (unchanged ? "true" : "false") << "\n";
        out.flush();
        if (!out || !unchanged) throw std::runtime_error("result write failed or loaded executable code changed");
        if (loaded != modules.size()) throw std::runtime_error("one or more provider modules failed to load; see result file");
        std::cout << "PASS factory-only scan; modules=" << loaded << ", provider pairs=" << providerPairs
                  << ", unresolved=" << unresolved << ", executable code unchanged=true\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return 1;
    }
}
