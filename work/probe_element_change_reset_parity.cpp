#include <windows.h>
#include "element_change_reset_candidate.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <stdexcept>
struct Result {std::uint32_t state,packed;};
#ifdef _M_IX86
static void* originalRoutine() {
    std::ifstream f("F:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);
    std::vector<char> file((std::istreambuf_iterator<char>(f)),{});
    if(file.size()<4096)throw std::runtime_error("missing DLL");
    auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());
    auto* nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);
    const auto* sections=IMAGE_FIRST_SECTION(nt);
    const unsigned rva=0x35dbd0;
    for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i) {
        const auto& s=sections[i];
        if(rva>=s.VirtualAddress && rva+9<=s.VirtualAddress+s.SizeOfRawData) {
            auto* code=VirtualAlloc(nullptr,9,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
            if(!code)throw std::runtime_error("allocation failed");
            std::memcpy(code,file.data()+s.PointerToRawData+rva-s.VirtualAddress,9);
            DWORD previous=0;
            if(!VirtualProtect(code,9,PAGE_EXECUTE_READ,&previous))throw std::runtime_error("protection failed");
            FlushInstructionCache(GetCurrentProcess(),code,9);return code;
        }
    }
    throw std::runtime_error("routine not found");
}
#endif
struct ResetAdapter {
    auto state(Result* p) {return p->state;}
    auto packed(Result* p) {return p->packed;}
    void setState(Result* p,std::uint32_t v) {p->state=v;}
    void setPacked(Result* p,std::uint32_t v) {p->packed=v;}
};
int main(int argc,char** argv) {try {
    if(argc!=2)throw std::runtime_error("oracle path required");
#ifdef _M_IX86
    if(std::ifstream(argv[1],std::ios::binary).good())throw std::runtime_error("oracle exists");
    auto* code=originalRoutine();
    using Native=void(__thiscall*)(void*);
    std::ofstream stream(argv[1],std::ios::binary);
#else
    std::ifstream stream(argv[1],std::ios::binary);
#endif
    if(!stream)throw std::runtime_error("oracle unavailable");
    unsigned checks=0;
    for(unsigned state=0;state<256;++state)
    for(unsigned low=0;low<256;++low)
    for(auto high:{0u,0x80000000u,0xffffff00u,0x12345600u}) {
        const auto packed=high|low;
        Result actual{};
#ifdef _M_IX86
        unsigned char element[0x44];std::memset(element,0xa5,sizeof(element));
        element[0x3e]=static_cast<unsigned char>(state);
        std::memcpy(element+0x40,&packed,4);
        reinterpret_cast<Native>(code)(element);
        actual.state=element[0x3e];std::memcpy(&actual.packed,element+0x40,4);
        for(unsigned i=0;i<sizeof(element);++i)
            if(i!=0x3e && !(i>=0x40 && i<0x44) && element[i]!=0xa5)
                throw std::runtime_error("original touched adjacent field");
        stream.write(reinterpret_cast<const char*>(&actual),sizeof(actual));
#else
        actual={state,packed};ResetAdapter adapter;
        resetElementChangeCandidate(&actual,adapter);
        Result expected{};stream.read(reinterpret_cast<char*>(&expected),sizeof(expected));
        if(!stream || std::memcmp(&actual,&expected,sizeof(actual)))
            throw std::runtime_error("change reset mismatch");
#endif
        ++checks;
    }
#ifdef _M_IX86
    stream.close();VirtualFree(code,0,MEM_RELEASE);
#else
    if(stream.peek()!=std::char_traits<char>::eof())throw std::runtime_error("oracle trailing bytes");
#endif
    std::cout<<"change reset original parity cases="<<checks<<" passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

