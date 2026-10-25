#include "source_mdl_virtual_sequences_candidate.h"
#include <fstream>
#include <iostream>
#include <iterator>
#ifdef NATIVE_VIRTUAL_SEQUENCE_LOOKUP
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
static std::vector<std::vector<uint8_t>>headers;
static void*__fastcall groupHeader(void*,void*,uint32_t group){return headers.at(group).data();}
#endif
int main(int argc,char**argv){try{
    if(argc!=4||std::ifstream(argv[3],std::ios::binary).good())throw std::runtime_error("ifm.dll lookup-queries fresh-output required");
    std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("lookup IO");
#ifdef NATIVE_VIRTUAL_SEQUENCE_LOOKUP
    static_assert(sizeof(void*)==4,"native lookup x86");NativePrivateWorldImageFixture image(argv[1]);auto*p=image.base+0x5da230;*p=0xe9;const int32_t delta=int32_t(reinterpret_cast<intptr_t>(&groupHeader)-reinterpret_cast<intptr_t>(p)-5);std::memcpy(p+1,&delta,4);image.seal();
#else
    static_assert(sizeof(void*)==8,"owned lookup x64");
#endif
    uint32_t count=0;size_t cases=0,queries=0;
    while(in.read(reinterpret_cast<char*>(&count),4)){
        if(!count||count>64)throw std::runtime_error("lookup group count");std::vector<uint32_t>locals(count);in.read(reinterpret_cast<char*>(locals.data()),std::streamsize(count*4));for(auto n:locals)if(n>4096)throw std::runtime_error("lookup local count");
        uint32_t entries=0;in.read(reinterpret_cast<char*>(&entries),4);if(!entries||entries>4096)throw std::runtime_error("lookup entry count");SourceVirtualSequencesCandidate table;table.entries.resize(entries);in.read(reinterpret_cast<char*>(table.entries.data()),std::streamsize(entries*16));for(const auto&e:table.entries)if(e.group>=count||e.index>=locals[e.group])throw std::runtime_error("lookup entry span");
        uint32_t requests=0;in.read(reinterpret_cast<char*>(&requests),4);if(!requests||requests>8192)throw std::runtime_error("lookup request count");std::vector<int32_t>indices(requests);in.read(reinterpret_cast<char*>(indices.data()),std::streamsize(requests*4));if(!in)throw std::runtime_error("lookup truncated");
#ifdef NATIVE_VIRTUAL_SEQUENCE_LOOKUP
        headers.clear();headers.resize(count);for(uint32_t group=0;group<count;++group){auto&b=headers[group];b.resize(0x200+size_t(locals[group])*212);const uint32_t start=0x200;std::memcpy(b.data()+0xbc,&locals[group],4);std::memcpy(b.data()+0xc0,&start,4);}
        std::array<uintptr_t,6>virtualModel{};virtualModel[2]=reinterpret_cast<uintptr_t>(table.entries.data());virtualModel[5]=entries;std::array<uintptr_t,2>wrapper{reinterpret_cast<uintptr_t>(headers[0].data()),reinterpret_cast<uintptr_t>(virtualModel.data())};const auto before=table.entries;const auto beforeHeaders=headers;
#endif
        for(int32_t index:indices){std::array<uint32_t,2>selected{};
#ifdef NATIVE_VIRTUAL_SEQUENCE_LOOKUP
            using Lookup=void*(__thiscall*)(void*,int32_t);const auto descriptor=reinterpret_cast<Lookup>(image.base+0x5da450)(wrapper.data(),index);bool found=false;
            for(uint32_t group=0;group<count;++group)for(uint32_t local=0;local<locals[group];++local)if(descriptor==headers[group].data()+0x200+size_t(local)*212){if(found)throw std::runtime_error("ambiguous descriptor");selected={group,local};found=true;}
            if(!found)throw std::runtime_error("descriptor outside groups");
#else
            const auto entry=sourceVirtualSequenceCandidate(table,index);selected={entry.group,entry.index};
#endif
            out.write(reinterpret_cast<const char*>(selected.data()),8);++queries;
        }
#ifdef NATIVE_VIRTUAL_SEQUENCE_LOOKUP
        if(headers!=beforeHeaders||std::memcmp(before.data(),table.entries.data(),entries*16))throw std::runtime_error("lookup input mutation");
#endif
        ++cases;
    }
    if(!in.eof()||in.gcount()||!cases)throw std::runtime_error("lookup query shape");out.flush();if(!out)throw std::runtime_error("lookup output");
#ifdef NATIVE_VIRTUAL_SEQUENCE_LOOKUP
    image.verify();VirtualFree(image.base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS lookup_tables="<<cases<<" requests="<<queries<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
