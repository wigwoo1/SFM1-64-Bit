#include <windows.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include "element_owner_path_candidate.h"
#ifdef OWNER_SYMBOL_HASH
#include "owner_symbol_hash_candidate.h"
#include "owner_symbol_installed_table_candidate.h"
#endif
static std::vector<std::string> cases() {
    std::vector<std::string> out{"",".","..","/.","A./B","A/../B","./A//../B"};
#ifdef OWNER_SYMBOL_HASH
    for(unsigned a=1;a<256;++a) {
        out.push_back(std::string(1,static_cast<char>(a)));
        for(unsigned b=1;b<256;b+=17) {
            std::string pair;pair.push_back(static_cast<char>(a));pair.push_back(static_cast<char>(b));
            out.push_back(pair);
        }
    }
#endif
#ifdef LOCAL_OWNER_NORMALIZATION
    out.push_back(std::string(259,'A'));
    out.push_back(std::string(260,'B'));
    out.push_back(std::string(400,'C')+"/../D");
    out.push_back(std::string("A/")+static_cast<char>(0x80)+"/B");
#endif
    for (const auto* prefix:{"","C:/","/","//server/"})
        for (const auto* a:{"A",".","..","A.","A..",""})
            for (const auto* b:{"B",".","..","B.","B..",""})
                for (const auto* suffix:{"","/C","//C","\\C"})
                    out.push_back(std::string(prefix)+a+"/"+b+suffix);
    return out;
}
#ifdef _M_IX86
static void writeString(std::ofstream& out,const std::string& s) {
    const auto size=static_cast<std::uint32_t>(s.size());
    out.write(reinterpret_cast<const char*>(&size),4);out.write(s.data(),size);
}
#else
static std::string readString(std::ifstream& in) {
    std::uint32_t size=0;in.read(reinterpret_cast<char*>(&size),4);
    if (!in || size>4096) throw std::runtime_error("bad oracle string");
    std::string s(size,'\0');in.read(s.data(),size);
    if (!in) throw std::runtime_error("short oracle");return s;
}
#endif
#ifdef _M_IX86
static unsigned diagnosticCount=0;
#ifdef ABSOLUTE_OWNER_PATH
static void __cdecl observedDiagnostic(const char*,...) {++diagnosticCount;}
#endif
static void* __cdecl observedMemmove(void* dest,const void* source,std::size_t size) {
    return std::memmove(dest,source,size);
}
static void mapOriginal() {
    std::ifstream f("F:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\tools\\ifm.dll",std::ios::binary);
    std::vector<char> file((std::istreambuf_iterator<char>(f)),{});
    if (file.size()<4096) throw std::runtime_error("missing DLL");
    const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());
    const auto* nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);
    if (nt->OptionalHeader.ImageBase!=0x10000000) throw std::runtime_error("unexpected base");
    auto* base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),
        nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    if (base!=reinterpret_cast<char*>(0x10000000)) throw std::runtime_error("mapping failed");
    const auto* section=IMAGE_FIRST_SECTION(nt);
    for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i) {
        if (section[i].PointerToRawData+section[i].SizeOfRawData>file.size())
            throw std::runtime_error("section out of bounds");
        std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,
                    section[i].SizeOfRawData);
    }
    // Only external dependency of10975f80 is bundled memmove. Intercept it in
    // private mapped memory; installed file is untouched, entrypoint not called.
    auto* target=reinterpret_cast<unsigned char*>(0x10a161e0);
    const auto displacement=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(
        &observedMemmove))-(0x10a161e0u+5u);
    target[0]=0xe9;std::memcpy(target+1,&displacement,4);
#ifdef ABSOLUTE_OWNER_PATH
    *reinterpret_cast<void**>(0x10a7e474)=reinterpret_cast<void*>(&observedDiagnostic);
#endif
#ifdef LOCAL_OWNER_NORMALIZATION
    // Explicitly select original ASCII fast path; locale branch not exercised.
    *reinterpret_cast<std::uint32_t*>(0x10fdb1c0)=0;
#endif
    FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
}
#endif
#ifdef ABSOLUTE_OWNER_PATH
struct AbsoluteOracleAdapter {
    unsigned diagnostics=0;
    bool currentDirectory(char*,int) {throw std::runtime_error("unexpected CWD call");}
    void diagnostic(const char*) {++diagnostics;}
};
#endif
int main(int argc,char** argv) {try {
    if(argc!=2)throw std::runtime_error("oracle path required");
    unsigned checks=0;
#ifdef _M_IX86
    if(std::ifstream(argv[1],std::ios::binary).good())throw std::runtime_error("oracle already exists");
    mapOriginal();std::ofstream out(argv[1],std::ios::binary);
    if(!out)throw std::runtime_error("oracle output unavailable");
    using Native=bool(__cdecl*)(char*,char);
    for(const auto& input:cases())for(char separator:{'\\','/'}) {
#ifdef OWNER_SYMBOL_HASH
        using HashNative=std::uint32_t(__fastcall*)(const char*);
        const auto expected=std::to_string(reinterpret_cast<HashNative>(0x109827f0)(input.c_str()));
        writeString(out,input);out.put(separator);out.put(1);writeString(out,expected);
        ++checks;continue;
#elif defined(ABSOLUTE_OWNER_PATH)
        std::vector<char> buffer(1024,static_cast<char>(0x55));
        diagnosticCount=0;
        using AbsoluteNative=void(__cdecl*)(char*,int,const char*,const char*);
        reinterpret_cast<AbsoluteNative>(0x10976910)(buffer.data(),260,input.c_str(),"C:/Fixture");
        const bool result=diagnosticCount!=0;
#elif defined(LOCAL_OWNER_NORMALIZATION)
        std::vector<char> buffer(260,static_cast<char>(0x55));
        using LocalNative=void(__cdecl*)(char*,int,const char*);
        reinterpret_cast<LocalNative>(0x10976270)(buffer.data(),260,input.c_str());
        bool result=true;
#else
        std::vector<char> buffer(input.begin(),input.end());buffer.push_back(0);
        bool result=reinterpret_cast<Native>(0x10975f80)(buffer.data(),separator);
#endif
#ifndef OWNER_SYMBOL_HASH
        writeString(out,input);out.put(separator);out.put(result?1:0);
        writeString(out,buffer.data());++checks;
#endif
    }
    if(!out)throw std::runtime_error("oracle write failed");
    std::cout<<"original_x86_cases="<<checks<<" DLL_entrypoint_not_called=1 memmove_intercepted=1\n";
#else
    static_assert(sizeof(void*)==8);
    std::ifstream in(argv[1],std::ios::binary);
    if(!in)throw std::runtime_error("oracle unavailable");
    while(in.peek()!=EOF) {
        const auto input=readString(in);const char separator=static_cast<char>(in.get());
        const auto result=in.get();const auto expected=readString(in);
#ifdef OWNER_SYMBOL_HASH
        (void)separator;(void)result;
        const auto actualHash=std::to_string(hashOwnerSymbolCandidate(input.c_str(),ownerSymbolInstalledTableCandidate));
        if(actualHash!=expected)throw std::runtime_error("hash mismatch");
        ++checks;continue;
#elif defined(ABSOLUTE_OWNER_PATH)
        (void)separator;
        std::vector<char> buffer(1024,static_cast<char>(0x55));
        AbsoluteOracleAdapter adapter;
        makeElementOwnerAbsolutePathCandidate(buffer.data(),260,input.c_str(),"C:/Fixture",adapter);
        const bool actual=adapter.diagnostics!=0;
#elif defined(LOCAL_OWNER_NORMALIZATION)
        (void)separator;
        std::vector<char> buffer(260,static_cast<char>(0x55));
        normalizeElementOwnerLocalAsciiCandidate(buffer.data(),260,input.c_str());
        const bool actual=true;
#else
        std::vector<char> buffer(input.begin(),input.end());buffer.push_back(0);
        const bool actual=cleanupElementOwnerPathCandidate(buffer.data(),separator);
#endif
#ifndef OWNER_SYMBOL_HASH
        if(actual!=(result!=0) || expected!=buffer.data()) {
            std::cerr<<"mismatch input="<<input<<" expected="<<expected
                     <<" actual="<<buffer.data()<<"\n";return 2;
        }
        ++checks;
#endif
    }
    if(checks!=cases().size()*2)throw std::runtime_error("oracle count mismatch");
    std::cout<<"x64_cases="<<checks<<" results_and_strings_match=1\n";
#endif
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
