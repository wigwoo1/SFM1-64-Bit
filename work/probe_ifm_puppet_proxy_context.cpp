// Isolated original IFM interface methods; no SFM process, DllMain or imports.
// Storage is a controlled shell using the statically verified CDmePuppet
// vtables. This does not execute the full puppet constructor or draw pipeline.
#define NOMINMAX
#include <windows.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>
#include "native_private_world_image_fixture.h"
static void need(bool ok,const char*message){if(!ok)throw std::runtime_error(message);}
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){
    if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;
    std::cerr<<"IFM interface child fault eip="<<std::hex<<p->ContextRecord->Eip<<'\n';ExitProcess(90);
}
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==4,"Original private IFM interface measurement is x86 only");
    need(argc==3&&!std::ifstream(argv[2]).good(),"read-only IFM fresh-output");
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    auto handler=AddVectoredExceptionHandler(1,&fault);
    NativePrivateWorldImageFixture image(argv[1]);image.seal();
    std::ofstream output(argv[2],std::ios::binary);need(bool(output),"Output open");
    using Method=void*(__thiscall*)(void*);
    for(uint32_t i=0;i<32;++i){
        std::array<uint8_t,512>storage{};
        for(size_t j=0;j<storage.size();++j)storage[j]=uint8_t(i*17u+uint32_t(j));
        auto*object=storage.data()+16+8*i;
        const uint32_t unknownTable=0x10adb9b8,renderTable=0x10adb904;
        std::memcpy(object,&unknownTable,4);std::memcpy(object+4,&renderTable,4);
        const auto before=storage;
        auto*renderable=object+4;
        auto*renderMethods=*reinterpret_cast<uint32_t**>(renderable);
        need(renderMethods[0]==0x10242660,"Original renderable vtable identity");
        auto*unknown=reinterpret_cast<Method>(renderMethods[0])(renderable);
        need(unknown==object,"Actual GetIClientUnknown receiver adjustment");
        auto*unknownMethods=*static_cast<uint32_t**>(unknown);
        need(unknownMethods[5]==0x10242670&&unknownMethods[7]==0x1022b000,"Original unknown vtable identities");
        auto*back=reinterpret_cast<Method>(unknownMethods[5])(unknown);
        auto*entity=reinterpret_cast<Method>(unknownMethods[7])(unknown);
        need(back==renderable&&entity==nullptr&&before==storage,"Original puppet interface result and no mutation");
        const std::array<uint32_t,5>record{i,uint32_t(unknown==object),uint32_t(back==renderable),uint32_t(entity==nullptr),uint32_t(before==storage)};
        output.write(reinterpret_cast<const char*>(record.data()),sizeof(record));image.verify();
    }
    output.flush();need(bool(output),"Complete output");image.verify();
    need(VirtualFree(image.base,0,MEM_RELEASE)!=0,"Private IFM image release");image.base=nullptr;
    RemoveVectoredExceptionHandler(handler);
    std::cout<<"PASS puppet_interface_cases=32 base_entity_null=32 unchanged_storage=32\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
