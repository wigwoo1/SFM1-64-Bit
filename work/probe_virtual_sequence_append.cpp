#include "source_mdl_virtual_sequences_candidate.h"
#include <fstream>
#include <iostream>
#include <iterator>
#ifdef NATIVE_VIRTUAL_SEQUENCE_CACHE
#include <map>
#include <cstdlib>
#endif
#ifdef NATIVE_VIRTUAL_SEQUENCE_APPEND
#define NOMINMAX
#include <windows.h>
#include "native_private_mdlcache_fixture.h"
static std::array<uint32_t,4096*16>temporary{};
static std::vector<std::vector<uint8_t>>headers;
static uint8_t*groupBase=nullptr;
static std::vector<std::array<uint32_t,4096>>maps;
static void*__fastcall header(void*self,void*){const auto at=static_cast<uint8_t*>(self)-groupBase;if(at<0||at%144||size_t(at/144)>=headers.size())throw std::runtime_error("group header lookup");return headers[size_t(at/144)].data();}
static int32_t __fastcall initializeEntries(void*self,void*,int32_t index,int32_t count){
    if(index||count<0||count>4096)throw std::runtime_error("temporary sequence vector request");auto*v=static_cast<uint32_t*>(self);v[0]=reinterpret_cast<uint32_t>(temporary.data());v[1]=4096;v[2]=0xffffffff;v[3]=uint32_t(count);v[4]=v[0];return index;
}
static int32_t __fastcall initializeMap(void*self,void*,int32_t index,int32_t count){
    const auto at=static_cast<uint8_t*>(self)-groupBase-0x2c;if(index||count<0||count>4096||at<0||at%144||size_t(at/144)>=maps.size())throw std::runtime_error("local sequence vector request");
    auto*v=static_cast<uint32_t*>(self);v[0]=reinterpret_cast<uint32_t>(maps[size_t(at/144)].data());v[1]=4096;v[2]=0xffffffff;v[3]=uint32_t(count);v[4]=v[0];return index;
}
static void redirect(uint8_t*base,size_t at,void*target){auto*p=base+at;*p=0xe9;const int32_t d=int32_t(reinterpret_cast<intptr_t>(target)-reinterpret_cast<intptr_t>(p)-5);std::memcpy(p+1,&d,4);}
#ifdef NATIVE_VIRTUAL_SEQUENCE_CACHE
// Test allocator ABI only. All native tree construction/search/insertion and
// label comparison/copy code execute; never supply a precomputed cache answer.
static std::map<void*,size_t>allocations;static size_t allocatedBytes=0,allocationCalls=0;
static void*__fastcall allocate(void*,void*,size_t bytes){
    if(!bytes||bytes>8*1024*1024||allocatedBytes+bytes>8*1024*1024)throw std::runtime_error("cache allocation limit");void*p=std::malloc(bytes);if(!p)throw std::runtime_error("cache allocation failed");allocations.emplace(p,bytes);allocatedBytes+=bytes;++allocationCalls;return p;
}
static void*__fastcall reallocate(void*self,void*unused,void*p,size_t bytes){
    if(!p)return allocate(self,unused,bytes);const auto it=allocations.find(p);if(it==allocations.end()||!bytes||bytes>8*1024*1024||allocatedBytes-it->second+bytes>8*1024*1024)throw std::runtime_error("cache reallocation contract");const auto old=it->second;void*result=std::realloc(p,bytes);if(!result)throw std::runtime_error("cache reallocation failed");allocations.erase(it);allocations.emplace(result,bytes);allocatedBytes=allocatedBytes-old+bytes;++allocationCalls;return result;
}
static void __fastcall release(void*,void*,void*p){if(!p)return;const auto it=allocations.find(p);if(it==allocations.end())throw std::runtime_error("unknown cache allocation");allocatedBytes-=it->second;allocations.erase(it);std::free(p);}
#endif
#endif
int main(int argc,char**argv){try{
    if(argc!=4||std::ifstream(argv[3],std::ios::binary).good())throw std::runtime_error("datacache.dll queries fresh-output required");
    std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("sequence IO");
#ifdef NATIVE_VIRTUAL_SEQUENCE_APPEND
    static_assert(sizeof(void*)==4,"x86 reference");NativePrivateMdlCacheFixture image(argv[1]);
    redirect(image.base,0xf5c0,reinterpret_cast<void*>(&header));redirect(image.base,0x11180,reinterpret_cast<void*>(&initializeEntries));redirect(image.base,0xfa40,reinterpret_cast<void*>(&initializeMap));
    *reinterpret_cast<uintptr_t*>(image.base+0x2a040)=reinterpret_cast<uintptr_t>(&GetCurrentThreadId);*reinterpret_cast<int32_t*>(image.base+0x32860)=-1;
#ifdef NATIVE_VIRTUAL_SEQUENCE_CACHE
    std::array<uintptr_t,6>allocatorTable{};allocatorTable[1]=reinterpret_cast<uintptr_t>(&allocate);allocatorTable[3]=reinterpret_cast<uintptr_t>(&reallocate);allocatorTable[5]=reinterpret_cast<uintptr_t>(&release);
    uintptr_t allocatorObject=reinterpret_cast<uintptr_t>(allocatorTable.data()),allocatorPointer=reinterpret_cast<uintptr_t>(&allocatorObject);*reinterpret_cast<uintptr_t*>(image.base+0x2a12c)=reinterpret_cast<uintptr_t>(&allocatorPointer);
#endif
    image.seal();
#else
    static_assert(sizeof(void*)==8,"owned x64");
#endif
    uint32_t count=0;size_t cases=0;
    while(in.read(reinterpret_cast<char*>(&count),4)){
        if(!count||count>64)throw std::runtime_error("group count");std::vector<SourceMdlSequenceMetadataCandidate>groups(count);
        for(auto&g:groups){uint32_t n=0;in.read(reinterpret_cast<char*>(&n),4);if(n>4096)throw std::runtime_error("local count");g.sequences.resize(n);
            for(auto&s:g.sequences){uint32_t length=0;in.read(reinterpret_cast<char*>(&s.flags),4);in.read(reinterpret_cast<char*>(&s.activity),4);in.read(reinterpret_cast<char*>(&length),4);if(!length||length>1024)throw std::runtime_error("label length");s.label.resize(length);in.read(s.label.data(),length);sourceSequenceLabelIdentityCandidate(s.label);}
        }if(!in)throw std::runtime_error("sequence input truncated");
        SourceVirtualSequencesCandidate result;
#ifdef NATIVE_VIRTUAL_SEQUENCE_APPEND
        size_t total=0;for(const auto&g:groups)total+=g.sequences.size();if(total>4096)throw std::runtime_error("reference total limit");
        headers.clear();headers.resize(count);maps.clear();maps.resize(count);std::vector<uint8_t>groupStorage(count*144);groupBase=groupStorage.data();
        for(uint32_t i=0;i<count;++i){auto&b=headers[i];b.resize(0x200+groups[i].sequences.size()*212);const uint32_t n=uint32_t(groups[i].sequences.size()),start=0x200;std::memcpy(b.data()+0xbc,&n,4);std::memcpy(b.data()+0xc0,&start,4);
            for(size_t j=0;j<groups[i].sequences.size();++j){const auto&s=groups[i].sequences[j];const size_t at=0x200+j*212;const uint32_t relative=uint32_t(b.size()-at);std::memcpy(b.data()+at+4,&relative,4);std::memcpy(b.data()+at+12,&s.flags,4);std::memcpy(b.data()+at+16,&s.activity,4);b.insert(b.end(),s.label.begin(),s.label.end());b.push_back(0);}
        }
        const auto beforeHeaders=headers;std::array<SourceVirtualSequenceEntryCandidate,4096>entries{};std::array<uint32_t,24>object{};object[0]=GetCurrentThreadId();object[2]=reinterpret_cast<uint32_t>(entries.data());object[3]=4096;object[4]=0xffffffff;object[6]=object[2];object[0x58/4]=reinterpret_cast<uint32_t>(groupBase);
        using Append=void(__thiscall*)(void*,uint32_t,void*);
#ifdef NATIVE_VIRTUAL_SEQUENCE_CACHE
        if(!allocations.empty()||allocatedBytes)throw std::runtime_error("cache fixture leak before case");
        std::array<uint32_t,14>cache{};using ConstructCache=void(__thiscall*)(void*);reinterpret_cast<ConstructCache>(image.base+0x12f30)(cache.data());
        *reinterpret_cast<uintptr_t*>(image.base+0x3f148)=reinterpret_cast<uintptr_t>(cache.data());*reinterpret_cast<int32_t*>(image.base+0x32860)=0;
#endif
        for(uint32_t i=0;i<count;++i){reinterpret_cast<Append>(image.base+0x13060)(object.data(),i,headers[i].data());if(object[5]>4096)throw std::runtime_error("reference entry count");}
        if(headers!=beforeHeaders||object[1]!=0)throw std::runtime_error("reference header/lock mutation");result.entries.assign(entries.begin(),entries.begin()+object[5]);
        for(uint32_t i=0;i<count;++i)result.localToMaster.emplace_back(maps[i].begin(),maps[i].begin()+groups[i].sequences.size());
#ifdef NATIVE_VIRTUAL_SEQUENCE_CACHE
        // Fixture teardown frees only addresses owned by this process's host
        // allocator. Native cache destructor/lifetime is outside this test.
        for(const auto&a:allocations)std::free(a.first);allocations.clear();allocatedBytes=0;
        *reinterpret_cast<uintptr_t*>(image.base+0x3f148)=0;
#endif
#else
#ifdef OWNED_VIRTUAL_SEQUENCE_CACHE
        result=sourceVirtualSequencesCandidate(groups,SourceSequenceNameCacheModeCandidate::Enabled);
#else
        result=sourceVirtualSequencesCandidate(groups);
#endif
#endif
        const uint32_t n=uint32_t(result.entries.size());out.write(reinterpret_cast<const char*>(&n),4);out.write(reinterpret_cast<const char*>(result.entries.data()),std::streamsize(n*16));for(const auto&m:result.localToMaster)out.write(reinterpret_cast<const char*>(m.data()),std::streamsize(m.size()*4));++cases;
    }
    if(!in.eof()||in.gcount()||!cases)throw std::runtime_error("sequence query shape");out.flush();if(!out)throw std::runtime_error("sequence output");
#ifdef NATIVE_VIRTUAL_SEQUENCE_APPEND
    image.verify();
#endif
    std::cout<<"PASS sequence_append_cases="<<cases;
#ifdef NATIVE_VIRTUAL_SEQUENCE_CACHE
    std::cout<<" native_cache=constructed allocations="<<allocationCalls<<" remaining_allocations="<<allocations.size();
#endif
    std::cout<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
