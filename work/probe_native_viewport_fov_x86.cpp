#define NOMINMAX
#include <windows.h>
#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>

// Test oracle only: execute the installed FOV arithmetic in a private image
// mapping, never call its DLL entry point or start the original SFM process.
static void nativeFov(float radians, void* view) {
    __asm {
        push esi
        mov esi, view
        movss xmm0, radians
        mov eax, 0x10292051
        call eax
        pop esi
    }
}

int main(int argc, char** argv) {
    HANDLE output = INVALID_HANDLE_VALUE;
    void* mapped = nullptr;
    try {
        static_assert(sizeof(void*) == 4, "native oracle requires x86");
        if (argc != 2) throw std::runtime_error("fresh oracle output required");
        std::ifstream input(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll)", std::ios::binary);
        if (!input) throw std::runtime_error("installed ifm.dll missing");
        const std::vector<char> bytes((std::istreambuf_iterator<char>(input)), {});
        if (bytes.size() < sizeof(IMAGE_DOS_HEADER)) throw std::runtime_error("PE header missing");
        const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(bytes.data());
        if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0 ||
            size_t(dos->e_lfanew) + sizeof(IMAGE_NT_HEADERS32) > bytes.size())
            throw std::runtime_error("invalid PE header");
        const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(bytes.data() + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE || nt->FileHeader.Machine != IMAGE_FILE_MACHINE_I386 ||
            nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC || nt->OptionalHeader.ImageBase != 0x10000000)
            throw std::runtime_error("unexpected native image architecture/base");
        mapped = VirtualAlloc(reinterpret_cast<void*>(0x10000000), nt->OptionalHeader.SizeOfImage,
                              MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (mapped != reinterpret_cast<void*>(0x10000000)) throw std::runtime_error("native image address unavailable");
        const auto sections = IMAGE_FIRST_SECTION(nt);
        for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
            const auto& section = sections[i];
            if (size_t(section.PointerToRawData) + section.SizeOfRawData > bytes.size() ||
                size_t(section.VirtualAddress) + section.SizeOfRawData > nt->OptionalHeader.SizeOfImage)
                throw std::runtime_error("invalid native section bounds");
            std::memcpy(static_cast<char*>(mapped) + section.VirtualAddress,
                        bytes.data() + section.PointerToRawData, section.SizeOfRawData);
        }
        const unsigned char prefix[]{0xF3,0x0F,0x59,0x05,0x7C,0xA8,0xA8,0x10};
        if (std::memcmp(reinterpret_cast<void*>(0x10292051), prefix, sizeof(prefix)) ||
            *reinterpret_cast<unsigned char*>(0x1029208C) != 0x8B ||
            *reinterpret_cast<float*>(0x10AC88E4) != 0.75f)
            throw std::runtime_error("native slice fingerprint mismatch");
        // Return immediately after the original [esi+0x58] FOV store.
        // Only private process memory is patched; the file stays read-only.
        *reinterpret_cast<unsigned char*>(0x1029208C) = 0xC3;
        FlushInstructionCache(GetCurrentProcess(), mapped, nt->OptionalHeader.SizeOfImage);
        output = CreateFileA(argv[1], GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (output == INVALID_HANDLE_VALUE) throw std::runtime_error("oracle output must be fresh");
        size_t count = 0;
        for (float fov : {1.f,14.f,25.2f,60.f,90.f,120.f,170.f})
            for (float aspect : {.5f,1.f,4.f/3.f,16.f/9.f,2.f,4.f}) {
                std::array<unsigned char,256> view{};
                std::memcpy(view.data() + 0x88, &aspect, sizeof(aspect));
                nativeFov(fov * *reinterpret_cast<float*>(0x10A8BD00), view.data());
                float result = 0;
                std::memcpy(&result, view.data() + 0x58, sizeof(result));
                if (!std::isfinite(result) || result <= 0 || result >= 180)
                    throw std::runtime_error("native viewport FOV result");
                const std::array<float,3> record{fov,aspect,result};
                DWORD written = 0;
                if (!WriteFile(output, record.data(), sizeof(record), &written, nullptr) || written != sizeof(record))
                    throw std::runtime_error("native oracle write failed");
                if (fov == 14.f && aspect == 16.f/9.f)
                    std::cout << std::setprecision(9) << "native_benchmark_fov=" << result << '\n';
                ++count;
            }
        if (!CloseHandle(output)) throw std::runtime_error("oracle close failed");
        output = INVALID_HANDLE_VALUE;
        VirtualFree(mapped, 0, MEM_RELEASE);
        mapped = nullptr;
        std::cout << "native_viewport_fov_cases=" << count << " DLL_entrypoint_not_called=1 original_process_not_used=1\n";
        return 0;
    } catch (const std::exception& e) {
        if (output != INVALID_HANDLE_VALUE) CloseHandle(output);
        if (mapped) VirtualFree(mapped, 0, MEM_RELEASE);
        std::cerr << e.what() << '\n';
        return 1;
    }
}
