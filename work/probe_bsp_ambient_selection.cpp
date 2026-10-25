#include "source_bsp_ambient_selection_candidate.h"
#include "source_bsp_hdr_enable_candidate.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#ifdef SFM64_NATIVE_AMBIENT_SELECTION
#include <windows.h>
static uint32_t leafVersion=0,queryCount=0;
static int32_t hdrType=0;
static uint32_t capturedEnable=0;
__declspec(naked) static void testMapInitialize(){__asm{ret}}
__declspec(naked) static void testEnableRequest(){
    __asm{
        movzx eax,byte ptr [esp+4]
        mov capturedEnable,eax
        ret
    }
}
__declspec(naked) static void testLumpConstructor(){
    __asm{
        mov eax,leafVersion
        mov [ecx+8],eax
        ret 4
    }
}
__declspec(naked) static void testHdrQuery(){
    __asm{
        inc queryCount
        mov eax,hdrType
        ret
    }
}
struct PrivateEngineImage {
    uint8_t*base=nullptr;size_t length=0;std::vector<std::pair<const uint8_t*,std::vector<uint8_t>>>sealed;
    PrivateEngineImage(const char*path,bool hardwareMode,bool enableMode){
        std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("original engine read");std::vector<uint8_t>b((std::istreambuf_iterator<char>(f)),{});
        auto require=[&](bool v){if(!v)throw std::runtime_error("private PE32 bounds/identity");};
        require(b.size()>=sizeof(IMAGE_DOS_HEADER));auto*dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(b.data());require(dos->e_magic==IMAGE_DOS_SIGNATURE&&dos->e_lfanew>0&&size_t(dos->e_lfanew)<b.size()-sizeof(IMAGE_NT_HEADERS32));
        auto*nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(b.data()+dos->e_lfanew);require(nt->Signature==IMAGE_NT_SIGNATURE&&nt->FileHeader.Machine==IMAGE_FILE_MACHINE_I386&&nt->OptionalHeader.Magic==IMAGE_NT_OPTIONAL_HDR32_MAGIC&&nt->OptionalHeader.ImageBase==0x10000000);
        length=nt->OptionalHeader.SizeOfImage;require(length>=(hardwareMode?0x183c10u:0x694198u)&&length<=64*1024*1024&&nt->OptionalHeader.SizeOfHeaders<=b.size());
        base=static_cast<uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),length,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(base!=reinterpret_cast<void*>(0x10000000))throw std::runtime_error("private preferred-base image unavailable");
        std::memcpy(base,b.data(),nt->OptionalHeader.SizeOfHeaders);const auto*sections=IMAGE_FIRST_SECTION(nt);
        require(reinterpret_cast<const uint8_t*>(sections)+size_t(nt->FileHeader.NumberOfSections)*sizeof(IMAGE_SECTION_HEADER)<=b.data()+b.size());
        for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){const auto&s=sections[i];require(uint64_t(s.PointerToRawData)+s.SizeOfRawData<=b.size()&&uint64_t(s.VirtualAddress)+s.SizeOfRawData<=length);std::memcpy(base+s.VirtualAddress,b.data()+s.PointerToRawData,s.SizeOfRawData);}
        auto jump=[&](uint32_t address,void*target){base[address-0x10000000]=0xe9;const int32_t delta=int32_t(reinterpret_cast<uintptr_t>(target)-address-5);std::memcpy(base+address-0x10000000+1,&delta,4);};
        if(!hardwareMode)jump(0x100e3220,reinterpret_cast<void*>(&testLumpConstructor));
        auto capture=[&](uint32_t address,uint32_t value){auto*p=base+address-0x10000000;*p++=0xb8;std::memcpy(p,&value,4);p+=4;*p++=0x8b;*p++=0xe5;*p++=0x5d;*p=0xc3;};
        // Owned capture epilogues at the selected path entry points. Only the
        // original selection runs; actual loaders/error handler do not execute.
        if(!hardwareMode){capture(0x100e4a1d,2);capture(0x100e4a77,1);capture(0x100e4ae6,0);capture(0x100e49e2,3);}
        if(enableMode){
            // Only map policy executes. Header initialization is a no-op;
            // enabled-state notifications are captured, then cleanup is skipped.
            jump(0x100dfe80,reinterpret_cast<void*>(&testMapInitialize));
            jump(0x100e1e50,reinterpret_cast<void*>(&testEnableRequest));
            jump(0x100e3545,reinterpret_cast<void*>(0x100e354f));
        }
        for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i)if(sections[i].Characteristics&IMAGE_SCN_MEM_EXECUTE){DWORD old=0;const auto*start=base+sections[i].VirtualAddress;sealed.push_back({start,std::vector<uint8_t>(start,start+sections[i].Misc.VirtualSize)});require(VirtualProtect(base+sections[i].VirtualAddress,sections[i].Misc.VirtualSize,PAGE_EXECUTE_READ,&old)!=0);}
        FlushInstructionCache(GetCurrentProcess(),base,length);
    }
    void checkCodeUnchanged()const{for(const auto&s:sealed)if(std::memcmp(s.first,s.second.data(),s.second.size()))throw std::runtime_error("sealed oracle code changed");}
    ~PrivateEngineImage(){if(base)VirtualFree(base,0,MEM_RELEASE);}
};
#endif
int main(int argc,char**argv){try{
    const bool hardwareMode=argc>=4&&std::string(argv[3])=="--hardware-query";
    const bool enableMode=argc>=4&&std::string(argv[3])=="--enable-request";
    const bool specialMode=hardwareMode||enableMode;
#ifdef SFM64_NATIVE_AMBIENT_SELECTION
    static_assert(sizeof(void*)==4,"Reference oracle is x86 only");
    if((!specialMode&&argc!=4)||(specialMode&&argc!=5))throw std::runtime_error("usage: native queries.bin fresh-output.bin [--hardware-query|--enable-request] original.dll");
    PrivateEngineImage image(argv[specialMode?4:3],hardwareMode,enableMode);
    uintptr_t table[36]{};table[34]=reinterpret_cast<uintptr_t>(&testHdrQuery);table[35]=reinterpret_cast<uintptr_t>(&testHdrQuery);uintptr_t object=reinterpret_cast<uintptr_t>(table);
    if(!hardwareMode)*reinterpret_cast<uintptr_t*>(0x10694194)=reinterpret_cast<uintptr_t>(&object);
    using Select=uint32_t(__cdecl*)();auto select=reinterpret_cast<Select>(0x100e49b0);
#else
    static_assert(sizeof(void*)==8,"Owned implementation is x64 only");
    if((!specialMode&&argc!=3)||(specialMode&&argc!=4))throw std::runtime_error("usage: owned queries.bin fresh-output.bin [--hardware-query|--enable-request]");
#endif
    if(std::ifstream(argv[2],std::ios::binary).good())throw std::runtime_error("fresh workspace output required");
    std::ifstream input(argv[1],std::ios::binary);std::ofstream output(argv[2],std::ios::binary);if(!input||!output)throw std::runtime_error("test IO");
    if(enableMode){
        // version, three (override-active, header-length, override-length)
        // triples for53/54/55, mat_hdr_level, raw hardware HDR type.
        std::array<uint32_t,12>row{};size_t cases=0;
        while(input.read(reinterpret_cast<char*>(row.data()),sizeof(row))){
            std::array<int32_t,12>signedRow{};std::memcpy(signedRow.data(),row.data(),sizeof(row));
            uint32_t capable=0,requested=0,calls=0;
#ifdef SFM64_NATIVE_AMBIENT_SELECTION
            *reinterpret_cast<int32_t*>(0x105cde64)=signedRow[0];
            for(size_t i=0;i<3;++i){
                *reinterpret_cast<uint32_t*>(0x105cdb1c+i*28)=row[1+i*3];
                *reinterpret_cast<int32_t*>(0x105cdb30+i*28)=signedRow[3+i*3];
                *reinterpret_cast<int32_t*>(0x105ce1bc+i*16)=signedRow[2+i*3];
            }
            std::array<uint8_t,64>convar{};std::memcpy(convar.data()+0x30,&signedRow[10],4);
            *reinterpret_cast<uintptr_t*>(0x105cd2ec)=reinterpret_cast<uintptr_t>(convar.data());
            hdrType=signedRow[11];queryCount=0;capturedEnable=0xdeadbeef;
            using MapRequest=uint8_t(__cdecl*)(uint32_t,uint32_t);
            capable=reinterpret_cast<MapRequest>(0x100e34a0)(0,0);requested=capturedEnable;calls=queryCount;
#else
            const auto result=sourceBspHdrEnableRequestCandidate(signedRow[0],[&](unsigned lump){const size_t i=lump-53;return sourceEffectiveBspLumpLengthCandidate(row[1+i*3]!=0,signedRow[2+i*3],signedRow[3+i*3]);},[&]{return signedRow[10];},[&]{++calls;return signedRow[11];});
            capable=result.mapHasHdr;requested=result.enableRequested;
#endif
            const std::array<uint32_t,3>encoded{capable,requested,calls};output.write(reinterpret_cast<const char*>(encoded.data()),sizeof(encoded));++cases;
        }
        if(!input.eof()||input.gcount()||!cases)throw std::runtime_error("enable query shape");output.flush();if(!output)throw std::runtime_error("enable output failure");
#ifdef SFM64_NATIVE_AMBIENT_SELECTION
        image.checkCodeUnchanged();
#endif
        std::cout<<"PASS enable_request_cases="<<cases<<" native_state_commit=0\n";return 0;
    }
    if(hardwareMode){
        std::array<uint32_t,6>row{};size_t cases=0;
        while(input.read(reinterpret_cast<char*>(row.data()),sizeof(row))){
            int32_t emulate=0,dx=0,type=0;std::memcpy(&emulate,row.data(),4);std::memcpy(&dx,row.data()+2,4);std::memcpy(&type,row.data()+3,4);
            uint32_t answer=0,enabled=0,globalEnabled=0;
#ifdef SFM64_NATIVE_AMBIENT_SELECTION
            std::array<uint8_t,0x900>local{},global{};uintptr_t vtable[48]{};
            vtable[17]=0x100034f0;vtable[47]=0x100030a0;
            const auto tableAddress=reinterpret_cast<uintptr_t>(vtable);std::memcpy(local.data(),&tableAddress,4);std::memcpy(global.data(),&tableAddress,4);
            local[0x88c]=uint8_t(row[1]);global[0x88c]=uint8_t(row[5]);std::memcpy(local.data()+0x4ec,&dx,4);std::memcpy(local.data()+0x550,&type,4);
            std::array<uint8_t,64>convar{};std::memcpy(convar.data()+0x30,&emulate,4);
            *reinterpret_cast<uintptr_t*>(0x10183c0c)=reinterpret_cast<uintptr_t>(convar.data());
            auto*globalObject=row[4]?local.data():global.data();*reinterpret_cast<uintptr_t*>(0x1016d130)=reinterpret_cast<uintptr_t>(globalObject);
            using Query=int32_t(__thiscall*)(void*);const auto value=reinterpret_cast<Query>(0x10003940)(local.data());std::memcpy(&answer,&value,4);
            enabled=local[0x88c];globalEnabled=globalObject[0x88c];
#else
            struct Hardware {uint8_t local,global;int32_t dx,type;bool alias;
                void enableGlobalHdr(){if(alias)local=1;else global=1;}
                uint8_t hdrEnabled(){return local;}int32_t dxSupportLevel(){return dx;}int32_t hardwareHdrType(){return type;}
            }hardware{uint8_t(row[1]),uint8_t(row[5]),dx,type,row[4]!=0};
            const auto value=sourceHardwareHdrTypeCandidate(emulate,hardware);std::memcpy(&answer,&value,4);enabled=hardware.local;globalEnabled=hardware.alias?hardware.local:hardware.global;
#endif
            const std::array<uint32_t,3>result{answer,enabled,globalEnabled};output.write(reinterpret_cast<const char*>(result.data()),sizeof(result));++cases;
        }
        if(!input.eof()||input.gcount()||!cases)throw std::runtime_error("hardware query shape");output.flush();if(!output)throw std::runtime_error("hardware output failure");
#ifdef SFM64_NATIVE_AMBIENT_SELECTION
        image.checkCodeUnchanged();
#endif
        std::cout<<"PASS hardware_cases="<<cases<<" original_device_initialization=0\n";return 0;
    }
    size_t cases=0;std::array<uint32_t,5>row{};
    while(input.read(reinterpret_cast<char*>(row.data()),sizeof(row))){
        int32_t type=0,headerLength=0,overrideLength=0;std::memcpy(&type,row.data()+1,4);std::memcpy(&headerLength,row.data()+3,4);std::memcpy(&overrideLength,row.data()+4,4);
        uint32_t selection=0,calls=0;
#ifdef SFM64_NATIVE_AMBIENT_SELECTION
        leafVersion=row[0];hdrType=type;queryCount=0;
        *reinterpret_cast<uint32_t*>(0x105cdb54)=row[2];*reinterpret_cast<int32_t*>(0x105cdb68)=overrideLength;*reinterpret_cast<int32_t*>(0x105ce1dc)=headerLength;
        selection=select();calls=queryCount;
#else
        selection=uint32_t(sourceBspAmbientSelectionCandidate(row[0],[&]{++calls;return type;},[&]{return sourceEffectiveBspLumpLengthCandidate(row[2]!=0,headerLength,overrideLength);}));
#endif
        const std::array<uint32_t,2>result{selection,calls};output.write(reinterpret_cast<const char*>(result.data()),sizeof(result));++cases;
    }
    if(!input.eof()||input.gcount()||!cases)throw std::runtime_error("query shape");output.flush();if(!output)throw std::runtime_error("output failure");
#ifdef SFM64_NATIVE_AMBIENT_SELECTION
    image.checkCodeUnchanged();
#endif
    std::cout<<"PASS selection_cases="<<cases<<" original_loader_execution=0\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
