#include <windows.h>
#include "element_state_mark_candidate.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <stdexcept>
struct Result {std::uint32_t state,flags[3];};
#ifdef _M_IX86
static void* originalRoutine() {
    std::ifstream f("F:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);
    std::vector<char> file((std::istreambuf_iterator<char>(f)),{});
    if(file.size()<4096)throw std::runtime_error("missing DLL");
    auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());
    auto* nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);
    const auto* sections=IMAGE_FIRST_SECTION(nt);
    const unsigned rva=0x35dbb0;
    for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i) {
        const auto& s=sections[i];
        if(rva>=s.VirtualAddress && rva+32<=s.VirtualAddress+s.SizeOfRawData) {
            auto* code=VirtualAlloc(nullptr,32,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
            if(!code)throw std::runtime_error("allocation failed");
            std::memcpy(code,file.data()+s.PointerToRawData+rva-s.VirtualAddress,32);
            DWORD previous=0;
            if(!VirtualProtect(code,32,PAGE_EXECUTE_READ,&previous))throw std::runtime_error("protection failed");
            FlushInstructionCache(GetCurrentProcess(),code,32);return code;
        }
    }
    throw std::runtime_error("routine not found");
}
#endif
struct Attribute {std::uint16_t flags;Attribute* next;};
struct Element {std::uint8_t state;Attribute* first;};
struct Adapter {
    auto state(Element* e) {return e->state;}
    void setState(Element* e,std::uint8_t s) {e->state=s;}
    auto* firstAttribute(Element* e) {return e->first;}
    auto* nextAttribute(Attribute* p) {return p->next;}
    auto flags(Attribute* p) {return p->flags;}
    void setFlags(Attribute* p,std::uint16_t f) {p->flags=f;}
};
int main(int argc,char** argv) {try {
    if(argc!=2)throw std::runtime_error("oracle path required");
#ifdef _M_IX86
    if(std::ifstream(argv[1],std::ios::binary).good())throw std::runtime_error("oracle exists");
    auto* code=originalRoutine();using Native=void(__thiscall*)(void*);
    std::ofstream stream(argv[1],std::ios::binary);
#else
    std::ifstream stream(argv[1],std::ios::binary);
#endif
    if(!stream)throw std::runtime_error("oracle unavailable");
    unsigned checks=0;
    for(unsigned flags=0;flags<65536;++flags)
    for(unsigned pattern=0;pattern<2;++pattern)
    for(unsigned length=0;length<=3;++length) {
        const auto state=static_cast<std::uint8_t>(pattern?flags>>8:flags);
        const std::uint16_t initial[3]{static_cast<std::uint16_t>(flags),
            static_cast<std::uint16_t>(~flags),static_cast<std::uint16_t>((flags<<8)|(flags>>8))};
        Result actual{};
#ifdef _M_IX86
        struct RawAttribute {std::uint32_t next;unsigned char padding[12];std::uint16_t flags;unsigned char tail[2];};
        static_assert(sizeof(RawAttribute)==20);
        RawAttribute attrs[3];std::memset(attrs,0xa5,sizeof(attrs));
        for(unsigned i=0;i<3;++i) {
            attrs[i].flags=initial[i];attrs[i].next=i+1<length?
                static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&attrs[i+1])):0;
        }
        unsigned char element[0x40];std::memset(element,0xa5,sizeof(element));
        const std::uint32_t head=length?static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(attrs)):0;
        std::memcpy(element+0x30,&head,4);element[0x3e]=state;
        unsigned char beforeElement[sizeof(element)];std::memcpy(beforeElement,element,sizeof(element));
        RawAttribute beforeAttrs[3];std::memcpy(beforeAttrs,attrs,sizeof(attrs));
        reinterpret_cast<Native>(code)(element);actual.state=element[0x3e];
        for(unsigned i=0;i<sizeof(element);++i)
            if(i!=0x3e && element[i]!=beforeElement[i])throw std::runtime_error("adjacent element mutation");
        for(unsigned i=0;i<3;++i) {
            actual.flags[i]=attrs[i].flags;
            beforeAttrs[i].flags=attrs[i].flags;
        }
        if(std::memcmp(attrs,beforeAttrs,sizeof(attrs)))throw std::runtime_error("adjacent attribute mutation");
        stream.write(reinterpret_cast<const char*>(&actual),sizeof(actual));
#else
        Attribute attrs[3];
        for(unsigned i=0;i<3;++i)attrs[i]={initial[i],i+1<length?&attrs[i+1]:nullptr};
        Element element{state,length?attrs:nullptr};Adapter adapter;
        clearElementAttributeMarkCandidate(&element,adapter);actual.state=element.state;
        for(unsigned i=0;i<3;++i)actual.flags[i]=attrs[i].flags;
        Result expected{};stream.read(reinterpret_cast<char*>(&expected),sizeof(expected));
        if(!stream || std::memcmp(&actual,&expected,sizeof(actual)))throw std::runtime_error("attribute reset mismatch");
#endif
        ++checks;
    }
#ifdef _M_IX86
    stream.close();VirtualFree(code,0,MEM_RELEASE);
#else
    if(stream.peek()!=std::char_traits<char>::eof())throw std::runtime_error("oracle trailing bytes");
#endif
    std::cout<<"attribute mark reset original parity cases="<<checks<<" passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

