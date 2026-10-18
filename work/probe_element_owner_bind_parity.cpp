#include <windows.h>
#include "element_owner_bind_candidate.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <stdexcept>
struct Result {std::uint32_t live,metadata,pointer;};
#ifdef _M_IX86
static void* originalRoutine() {
    std::ifstream f("F:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);
    std::vector<char> file((std::istreambuf_iterator<char>(f)),{});
    if(file.size()<4096)throw std::runtime_error("missing DLL");
    auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());
    auto* nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);
    const auto* sections=IMAGE_FIRST_SECTION(nt);
    const unsigned rva=0x354b70;
    for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i) {
        const auto& s=sections[i];
        if(rva>=s.VirtualAddress && rva+75<=s.VirtualAddress+s.SizeOfRawData) {
            auto* code=VirtualAlloc(nullptr,75,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
            if(!code)throw std::runtime_error("allocation failed");
            std::memcpy(code,file.data()+s.PointerToRawData+rva-s.VirtualAddress,75);
            DWORD previous=0;
            if(!VirtualProtect(code,75,PAGE_EXECUTE_READ,&previous))throw std::runtime_error("protection failed");
            FlushInstructionCache(GetCurrentProcess(),code,75);return code;
        }
    }
    throw std::runtime_error("routine not found");
}
#endif
int main(int argc,char** argv) {try {
    if(argc!=2)throw std::runtime_error("oracle path required");
#ifdef _M_IX86
    if(std::ifstream(argv[1],std::ios::binary).good())throw std::runtime_error("oracle exists");
    auto* code=originalRoutine();
    using Native=void(__thiscall*)(void*,std::uint32_t,void*);
    std::ofstream stream(argv[1],std::ios::binary);
#else
    std::ifstream stream(argv[1],std::ios::binary);
#endif
    if(!stream)throw std::runtime_error("oracle unavailable");
    unsigned checks=0;
    for(auto meta:{0u,3u,2047u,2048u,0x80000000u,0x80000003u,0xffffffffu})
    for(auto generation:{0u,3u,2047u})for(unsigned index=0;index<4;++index)
    for(unsigned count=0;count<4;++count)for(auto high:{0u,0x80000000u})
    for(auto live:{0u,1u,0xffffffffu})for(auto object:{0u,0x1234u}) {
        const auto handle=high|(generation<<20)|index;
        Result actual;
#ifdef _M_IX86
        struct Entry {std::uint32_t metadata,pointer;} entries[4];
        for(auto& e:entries)e={meta,0x5678};
        std::uint32_t pool[5]={live,static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(entries)),4,0,count};
        reinterpret_cast<Native>(code)(pool,handle,reinterpret_cast<void*>(object));
        actual={pool[0],entries[index].metadata,entries[index].pointer};
        stream.write(reinterpret_cast<const char*>(&actual),sizeof(actual));
#else
        ElementOwnerPoolEntryCandidate entries[4];
        for(auto& e:entries)e={meta,reinterpret_cast<ElementOwnerCounterCandidate*>(std::uintptr_t(0x5678))};
        auto liveValue=live;
        bindElementOwnerCandidate(handle,entries,count,liveValue,
            reinterpret_cast<ElementOwnerCounterCandidate*>(std::uintptr_t(object)));
        actual={liveValue,entries[index].generationAndFlags,
            static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(entries[index].object))};
        Result expected{};stream.read(reinterpret_cast<char*>(&expected),sizeof(expected));
        if(!stream || std::memcmp(&actual,&expected,sizeof(actual)))throw std::runtime_error("binding mismatch");
#endif
        ++checks;
    }
#ifdef _M_IX86
    stream.close();VirtualFree(code,0,MEM_RELEASE);
#else
    if(stream.peek()!=std::char_traits<char>::eof())throw std::runtime_error("oracle trailing bytes");
#endif
    std::cout<<"owner binding cases="<<checks<<" passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
