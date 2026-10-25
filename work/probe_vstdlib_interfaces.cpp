#define NOMINMAX
#include <windows.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
using Factory = void* (__cdecl*)(const char*, int*);
int main(int argc, char** argv) {
    static_assert(sizeof(void*) == 4, "x86 factory-only check");
    if (argc != 3) { std::cerr << "usage: probe_vstdlib_interfaces <copied-bin> <output>\n"; return 2; }
    if (!SetDllDirectoryA(argv[1])) return 3;
    HMODULE module = LoadLibraryExA((std::string(argv[1]) + "\\vstdlib.dll").c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!module) { std::cerr << "LoadLibrary vstdlib failed " << GetLastError() << "\n"; return 4; }
    auto factory = reinterpret_cast<Factory>(GetProcAddress(module, "CreateInterface"));
    if (!factory) return 5;
    std::ofstream out(argv[2], std::ios::binary | std::ios::trunc);
    if (!out) return 6;
    out << "scope=CreateInterface queries only; no interface methods, Connect, Init, device creation, or rendering called\n";
    const char* names[] = {"VStdLib001", "VStdLib002", "VStdLib003", "VStdLib004", "VStdLib005", "VStdLib006", "VStdLib007", "VStdLib008", "VStdLib009", "VEngineCvar007", "VCvarQuery001", "EventSystem001", "VProcessUtils002"};
    for (const char* name : names) { int status = -1; void* value = factory(name, &status); out << name << " result=" << (value ? "present" : "null") << " status=" << status << "\n"; }
    out.close();
    FreeLibrary(module); SetDllDirectoryA(nullptr);
    std::cout << "factory queries completed\n";
    return 0;
}