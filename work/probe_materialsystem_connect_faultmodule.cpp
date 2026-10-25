#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

struct ImageGuard {
    std::uintptr_t base = 0;
    std::size_t size = 0;
    std::vector<std::pair<const std::uint8_t*, std::vector<std::uint8_t>>> code;
    explicit ImageGuard(HMODULE module) : base(reinterpret_cast<std::uintptr_t>(module)) {
        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE) throw std::runtime_error("invalid PE image");
        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
            throw std::runtime_error("expected copied x86 Source module");
        size = nt->OptionalHeader.SizeOfImage;
        for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
            const auto& sec = IMAGE_FIRST_SECTION(nt)[i];
            if (!(sec.Characteristics & IMAGE_SCN_MEM_EXECUTE) || !sec.Misc.VirtualSize) continue;
            const auto* p = reinterpret_cast<const std::uint8_t*>(base + sec.VirtualAddress);
            code.push_back({p, std::vector<std::uint8_t>(p, p + sec.Misc.VirtualSize)});
        }
    }
    void verify(const char* name) const {
        for (const auto& section : code)
            if (std::memcmp(section.first, section.second.data(), section.second.size()))
                throw std::runtime_error(std::string(name) + " executable code changed");
    }
};

using InterfaceFactory = void* (__cdecl*)(const char*, int*);
static InterfaceFactory g_shaderFactory = nullptr;
static InterfaceFactory g_filesystemFactory = nullptr;
static InterfaceFactory g_tier0Factory = nullptr;
static InterfaceFactory g_vstdlibFactory = nullptr;
static InterfaceFactory g_materialFactory = nullptr;
static std::ofstream* g_queryLog = nullptr;
static std::string g_exceptionPath;
static void* __cdecl queryApplicationInterface(const char* name, int* status) {
    const InterfaceFactory factories[] = {g_tier0Factory, g_vstdlibFactory, g_filesystemFactory, g_shaderFactory, g_materialFactory};
    const char* providers[] = {"tier0", "vstdlib", "filesystem", "shader", "materialsystem"};
    int localStatus = -999;
    void* value = nullptr;
    const char* provider = "none";
    for (unsigned i = 0; i < sizeof(factories) / sizeof(factories[0]); ++i) {
        if (!factories[i]) continue;
        localStatus = -999;
        value = factories[i](name, &localStatus);
        if (value) { provider = providers[i]; break; }
    }
    if (g_queryLog) *g_queryLog << "factory_request=" << name << " provider=" << provider << " result=" << (value ? "present" : "null") << " status=" << localStatus << "\n" << std::flush;
    if (status) *status = localStatus;
    return value;
}
static bool safeReadExceptionWord(std::uintptr_t address, std::uint32_t* value) {
    __try { *value = *reinterpret_cast<const volatile std::uint32_t*>(address); return true; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
static LONG WINAPI captureUnhandledException(EXCEPTION_POINTERS* info) {
    if (!info || !info->ExceptionRecord || !info->ContextRecord) return EXCEPTION_CONTINUE_SEARCH;
    char message[8192]{};
    const auto* context = info->ContextRecord;
    MEMORY_BASIC_INFORMATION memory{};
    const auto ip = static_cast<std::uintptr_t>(context->Eip);
    VirtualQuery(reinterpret_cast<const void*>(ip), &memory, sizeof(memory));
    const auto base = reinterpret_cast<std::uintptr_t>(memory.AllocationBase);
char faultModule[MAX_PATH]{};
    if (base) GetModuleFileNameA(reinterpret_cast<HMODULE>(base), faultModule, MAX_PATH);
    std::uint8_t instructionBytes[8]{};
    SIZE_T instructionRead = 0;
    ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(ip), instructionBytes, sizeof(instructionBytes), &instructionRead);
    int used = sprintf_s(message, sizeof(message), "unhandled_exception=0x%08lx eip=0x%08lx allocation_base=0x%08lx rva=0x%08lx module=%s instruction_bytes_read=%llu bytes=%02X %02X %02X %02X %02X %02X %02X %02X eax=0x%08lx ebx=0x%08lx ecx=0x%08lx edx=0x%08lx esi=0x%08lx edi=0x%08lx ebp=0x%08lx esp=0x%08lx\n",
        info->ExceptionRecord->ExceptionCode, static_cast<unsigned long>(ip), static_cast<unsigned long>(base), static_cast<unsigned long>(base ? ip - base : 0), faultModule, static_cast<unsigned long long>(instructionRead),
        instructionBytes[0], instructionBytes[1], instructionBytes[2], instructionBytes[3], instructionBytes[4], instructionBytes[5], instructionBytes[6], instructionBytes[7],
        context->Eax, context->Ebx, context->Ecx, context->Edx, context->Esi, context->Edi, context->Ebp, context->Esp);
    for (unsigned i = 0; i < 24 && used > 0 && used < static_cast<int>(sizeof(message)); ++i) {
        std::uint32_t word = 0;
        const auto address = static_cast<std::uintptr_t>(context->Esp) + i * sizeof(std::uint32_t);
        if (!safeReadExceptionWord(address, &word)) break;
        MEMORY_BASIC_INFORMATION target{};
        VirtualQuery(reinterpret_cast<const void*>(word), &target, sizeof(target));
        const auto targetBase = reinterpret_cast<std::uintptr_t>(target.AllocationBase);
        char modulePath[MAX_PATH]{};
        if (targetBase) GetModuleFileNameA(reinterpret_cast<HMODULE>(targetBase), modulePath, MAX_PATH);
        const int count = sprintf_s(message + used, sizeof(message) - used, "stack[%02u] addr=0x%08lx value=0x%08lx target_base=0x%08lx target_rva=0x%08lx module=%s\n",
            i, static_cast<unsigned long>(address), static_cast<unsigned long>(word), static_cast<unsigned long>(targetBase),
            static_cast<unsigned long>(targetBase ? word - targetBase : 0), modulePath);
        if (count <= 0) break;
        used += count;
    }
    if (used > 0 && !g_exceptionPath.empty()) {
        HANDLE file = CreateFileA(g_exceptionPath.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file != INVALID_HANDLE_VALUE) { DWORD written = 0; WriteFile(file, message, static_cast<DWORD>(used), &written, nullptr); CloseHandle(file); }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

int main(int argc, char** argv) {
    HMODULE tier0 = nullptr, vstdlib = nullptr, filesystem = nullptr, shader = nullptr, material = nullptr;
    try {
        static_assert(sizeof(void*) == 4, "reference lifecycle oracle must be x86");
        if (argc != 3) throw std::runtime_error("usage: probe_materialsystem_connect_lifecycle <copied-bin-dir> <output.txt>");
        const std::string dir = argv[1];
        if (!SetDllDirectoryA(dir.c_str())) throw std::runtime_error("SetDllDirectory failed");
        tier0 = LoadLibraryExA((dir + "\\tier0.dll").c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        vstdlib = LoadLibraryExA((dir + "\\vstdlib.dll").c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        filesystem = LoadLibraryExA((dir + "\\filesystem_stdio.dll").c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        shader = LoadLibraryExA((dir + "\\shaderapidx9.dll").c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        material = LoadLibraryExA((dir + "\\materialsystem.dll").c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (!tier0 || !vstdlib || !filesystem || !shader || !material)
            throw std::runtime_error("failed to load copied Source DLL set");
        ImageGuard shaderGuard(shader), materialGuard(material), filesystemGuard(filesystem);
        g_shaderFactory = reinterpret_cast<InterfaceFactory>(GetProcAddress(shader, "CreateInterface"));
        g_filesystemFactory = reinterpret_cast<InterfaceFactory>(GetProcAddress(filesystem, "CreateInterface"));
        g_tier0Factory = reinterpret_cast<InterfaceFactory>(GetProcAddress(tier0, "CreateInterface"));
        g_vstdlibFactory = reinterpret_cast<InterfaceFactory>(GetProcAddress(vstdlib, "CreateInterface"));
        g_materialFactory = reinterpret_cast<InterfaceFactory>(GetProcAddress(material, "CreateInterface"));
        if (!g_shaderFactory || !g_filesystemFactory || !g_materialFactory) throw std::runtime_error("required CreateInterface export missing");
        SetUnhandledExceptionFilter(&captureUnhandledException);
        int status = -1;
        void* object = g_materialFactory("VMaterialSystem080", &status);
        if (!object || status) throw std::runtime_error("VMaterialSystem080 factory did not return the primary material system");
        int filesystemStatus = -1;
        void* filesystemObject = g_filesystemFactory("VFileSystem017", &filesystemStatus);
        if (!filesystemObject || filesystemStatus) throw std::runtime_error("copied filesystem factory did not return VFileSystem017");
        auto** table = *reinterpret_cast<void***>(object);
        if (!table || !materialGuard.base || reinterpret_cast<std::uintptr_t>(table) - materialGuard.base != 0x984cc)
            throw std::runtime_error("material-system vtable identity mismatch");
        using Connect = bool (__thiscall*)(void*, InterfaceFactory);
        using Disconnect = void (__thiscall*)(void*);
        using SetShaderApi = void (__thiscall*)(void*, const char*);
        using FilesystemConnect = void (__cdecl*)(InterfaceFactory*, int);
        using FilesystemDisconnect = void (__cdecl*)();
        auto connect = reinterpret_cast<Connect>(table[0]);
        auto disconnect = reinterpret_cast<Disconnect>(table[1]);
        auto setShaderApi = reinterpret_cast<SetShaderApi>(table[9]);
        auto* shaderFactoryField = reinterpret_cast<InterfaceFactory*>(reinterpret_cast<std::uint8_t*>(object) + 0x29d8);
        if (!setShaderApi || *shaderFactoryField) throw std::runtime_error("unexpected initial shader API selection state");
        setShaderApi(object, nullptr); // The existing UI path selects the DX9 backend by default.
        const auto selectedFactory = reinterpret_cast<std::uintptr_t>(*shaderFactoryField);
        const auto shaderImageBase = reinterpret_cast<std::uintptr_t>(shader);
        if (!selectedFactory || selectedFactory < shaderImageBase || selectedFactory >= shaderImageBase + shaderGuard.size)
            throw std::runtime_error("SetShaderAPI did not install a factory inside the copied DX9 backend");
        const auto materialBase = reinterpret_cast<std::uintptr_t>(material);
        auto* filesystemGlobal = reinterpret_cast<void**>(materialBase + 0x1255c4);
        if (*filesystemGlobal) throw std::runtime_error("material filesystem global was not initially null");
        std::ofstream out(argv[2], std::ios::binary | std::ios::trunc);
        if (!out) throw std::runtime_error("cannot create lifecycle result");
        g_queryLog = &out;
        g_exceptionPath = std::string(argv[2]) + ".exception.txt";
        out << "scope=real copied-DLL internal filesystem connector, app-system Connect, app-system Disconnect, internal filesystem disconnect; Init/Shutdown/material queries/GPU device creation not called\n" << std::flush;
        auto filesystemConnect = reinterpret_cast<FilesystemConnect>(materialBase + 0x74fa0);
        auto filesystemDisconnect = reinterpret_cast<FilesystemDisconnect>(materialBase + 0x74fb0);
        filesystemConnect(&g_filesystemFactory, 1);
        out << "normal_filesystem_connector_global=0x" << std::hex << reinterpret_cast<std::uintptr_t>(*filesystemGlobal)
            << " expected_vfilesystem=0x" << reinterpret_cast<std::uintptr_t>(filesystemObject) << std::dec << "\n" << std::flush;
        if (*filesystemGlobal != filesystemObject) throw std::runtime_error("verified internal connector did not bind VFileSystem017");
        out << "normal_filesystem_connector=success\n" << std::flush;
        out << "set_shader_api=success selected_factory=0x" << std::hex << selectedFactory << " backend_base=0x" << shaderImageBase << std::dec << "\n" << std::flush;
        const bool connected = connect(object, &queryApplicationInterface);
        out << "connect_return=" << (connected ? "success" : "failure") << "\n" << std::flush;
        // The Source app-system disconnect path is the cleanup for Connect-owned state.
        disconnect(object);
        out << "disconnect_called=true\n" << std::flush;
        filesystemDisconnect();
        if (*filesystemGlobal) throw std::runtime_error("internal filesystem disconnect did not restore null global");
        out << "normal_filesystem_disconnect=success\n" << std::flush;
        materialGuard.verify("materialsystem.dll");
        shaderGuard.verify("shaderapidx9.dll");
        filesystemGuard.verify("filesystem_stdio.dll");
        out << "materialsystem_executable_sections_unchanged=true\nshaderapidx9_executable_sections_unchanged=true\nfilesystem_stdio_executable_sections_unchanged=true\n" << std::flush;
        if (!out) throw std::runtime_error("lifecycle result write failed");
        std::cout << "Connect=" << (connected ? "success" : "failure") << "; Disconnect completed; no Init or device creation\n";
        g_shaderFactory = nullptr;
        g_filesystemFactory = nullptr;
        g_tier0Factory = nullptr;
        g_vstdlibFactory = nullptr;
        g_materialFactory = nullptr;
        g_queryLog = nullptr;
        FreeLibrary(material); material = nullptr;
        FreeLibrary(shader); shader = nullptr;
        FreeLibrary(filesystem); filesystem = nullptr;
        FreeLibrary(vstdlib); vstdlib = nullptr;
        FreeLibrary(tier0); tier0 = nullptr;
        SetDllDirectoryA(nullptr);
        return connected ? 0 : 2;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        if (material) FreeLibrary(material);
        if (shader) FreeLibrary(shader);
        if (filesystem) FreeLibrary(filesystem);
        if (vstdlib) FreeLibrary(vstdlib);
        if (tier0) FreeLibrary(tier0);
        SetDllDirectoryA(nullptr);
        return 1;
    }
}





