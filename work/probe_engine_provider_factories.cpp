#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using InterfaceFactory = void* (__cdecl*)(const char*, int*);

struct LoadedModule {
    std::string name;
    HMODULE handle = nullptr;
    std::vector<std::pair<const std::uint8_t*, std::vector<std::uint8_t>>> executableSections;

    explicit LoadedModule(const char* filename) : name(filename) {
        handle = LoadLibraryExA(filename, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (!handle) throw std::runtime_error("failed to load copied provider module: " + name);
        const auto base = reinterpret_cast<std::uintptr_t>(handle);
        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE) throw std::runtime_error("invalid copied PE image: " + name);
        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
            throw std::runtime_error("provider is not an x86 PE image: " + name);
        for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
            const auto& section = IMAGE_FIRST_SECTION(nt)[i];
            if (!(section.Characteristics & IMAGE_SCN_MEM_EXECUTE) || !section.Misc.VirtualSize) continue;
            const auto* begin = reinterpret_cast<const std::uint8_t*>(base + section.VirtualAddress);
            executableSections.push_back({begin, std::vector<std::uint8_t>(begin, begin + section.Misc.VirtualSize)});
        }
    }

    InterfaceFactory factory() const {
        return reinterpret_cast<InterfaceFactory>(GetProcAddress(handle, "CreateInterface"));
    }

    void verifyCode() const {
        for (const auto& section : executableSections)
            if (std::memcmp(section.first, section.second.data(), section.second.size()))
                throw std::runtime_error("executable section changed in copied module: " + name);
    }

    ~LoadedModule() { if (handle) FreeLibrary(handle); }
    LoadedModule(const LoadedModule&) = delete;
    LoadedModule& operator=(const LoadedModule&) = delete;
    LoadedModule(LoadedModule&& other) noexcept
        : name(std::move(other.name)), handle(other.handle), executableSections(std::move(other.executableSections)) {
        other.handle = nullptr;
    }
    LoadedModule& operator=(LoadedModule&&) = delete;
};

int main(int argc, char** argv) {
    try {
        static_assert(sizeof(void*) == 4, "provider-factory reference probe must be x86");
        if (argc != 3) throw std::runtime_error("usage: probe_engine_provider_factories <copied-bin-dir> <output.txt>");
        if (!SetDllDirectoryA(argv[1])) throw std::runtime_error("SetDllDirectory failed");

        const char* moduleNames[] = {
            "tier0.dll", "vstdlib.dll", "filesystem_stdio.dll", "shaderapidx9.dll",
            "materialsystem.dll", "vphysics.dll", "inputsystem.dll", "datacache.dll", "vgui2.dll"
        };
        std::vector<LoadedModule> modules;
        for (const auto* name : moduleNames) modules.emplace_back(name);

        const char* interfaceNames[] = {
            "Physics2 Interface v0.3", "Physics2 Interface ActorMgr v0.1", "Physics2 Interface ResourceMgr v0.1",
            "ResourceSystem004", "VMaterialSystem2_001", "InputSystemVersion001", "InputStackSystemVersion001",
            "RenderDeviceMgr001", "SoundSystem001", "VP4002", "VMDLLIB001", "VResourceAccessControl001",
            "VPrecacheSystem001", "VStudioRender026", "VGUI_ivgui008", "VGUI_Input005", "VGUI_Panel009",
            "VGUI_Surface031", "VGUI_Scheme010", "VGUI_System010", "Localize_001", "MatSystemSurface006",
            "VDataCache003", "MDLCache004", "VAvi001", "VBik001", "VDmeMakeFileUtils001", "VPhysicsCollision007",
            "VSoundEmitter003", "MeshSystem001", "RenderDevice001", "RenderHardwareConfig001", "SceneSystem_001",
            "WorldRendererMgr001", "RenderSystemSurface001", "PropertyEditorSystem_Source1_001", "IVideoServices002",
            "MATCHFRAMEWORK_001", "GameUISystemMgr001", "NetworkSystemVersion001"
        };

        std::ofstream output(argv[2], std::ios::binary | std::ios::trunc);
        if (!output) throw std::runtime_error("cannot open factory result output");
        output << "scope=factory-only queries from copied modules; no returned interface methods called; no Init, Connect, device creation, or rendering\n";
        for (const auto& module : modules)
            output << "factory_export=" << module.name << " present=" << (module.factory() ? "true" : "false") << "\n";
        unsigned foundPairs = 0;
        unsigned missingNames = 0;
        for (const auto* interfaceName : interfaceNames) {
            unsigned foundForName = 0;
            for (const auto& module : modules) {
                auto factory = module.factory();
                if (!factory) continue;
                int status = -999;
                void* object = factory(interfaceName, &status);
                if (object) {
                    ++foundForName;
                    ++foundPairs;
                    auto** table = *reinterpret_cast<void***>(object);
                    const auto tableValue = reinterpret_cast<std::uintptr_t>(table);
                    const auto base = reinterpret_cast<std::uintptr_t>(module.handle);
                    const auto tableRva = tableValue >= base ? tableValue - base : 0;
                    output << "interface=" << interfaceName << " provider=" << module.name
                           << " status=" << status << " vtable_rva=0x" << std::hex << tableRva << std::dec << "\n";
                }
            }
            if (!foundForName) {
                ++missingNames;
                output << "interface=" << interfaceName << " provider=none status=1\n";
            }
        }
        for (const auto& module : modules) module.verifyCode();
        output << "summary=unique_interfaces:" << (sizeof(interfaceNames) / sizeof(interfaceNames[0]))
               << ",provider_pairs:" << foundPairs << ",unresolved_interfaces:" << missingNames
               << ",loaded_modules:" << modules.size() << ",all_executable_sections_unchanged:true\n";
        output.flush();
        if (!output) throw std::runtime_error("factory result write failed");
        std::cout << "PASS factory-only scan; unique interfaces=" << (sizeof(interfaceNames) / sizeof(interfaceNames[0]))
                  << " provider pairs=" << foundPairs << " unresolved=" << missingNames << "\n";
        SetDllDirectoryA(nullptr);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        SetDllDirectoryA(nullptr);
        return 1;
    }
}
