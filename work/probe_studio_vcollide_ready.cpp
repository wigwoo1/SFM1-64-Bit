#include <Windows.h>
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include "source_studio_vcollide_ready_query_candidate.h"
#ifdef NATIVE_STUDIO_VCOLLIDE_READY
#include "native_private_mdlcache_fixture.h"
static size_t activeCase=0;
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"cache fault case="<<activeCase<<" eip="<<std::hex<<p->ContextRecord->Eip<<'\n';ExitProcess(90);}
#endif
template<class T>static T at(const uint8_t*b,size_t p){T v;std::memcpy(&v,b+p,sizeof(v));return v;}
template<class T>static void set(uint8_t*b,size_t p,T v){std::memcpy(b+p,&v,sizeof(v));}
template<class T>static void put(std::ostream&o,const T&v){o.write(reinterpret_cast<const char*>(&v),sizeof(v));}
static std::vector<uint8_t>read(const char*p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("cache input read");return {(std::istreambuf_iterator<char>(f)),{}};}
struct Services {
    std::vector<uint16_t>groups;std::array<uint64_t,9>objects{};std::array<uint32_t,9>refs{};
    std::vector<uint16_t>virtualModelGroups(uint16_t){return groups;}
    uint64_t cachedGroupObject(uint16_t h){if(h<1||h>8)throw std::runtime_error("group fixture handle");return objects[h];}
    void retainCollisionObject(uint64_t id){for(size_t i=0;i<objects.size();++i)if(objects[i]==id){++refs[i];return;}throw std::runtime_error("unknown collision object retention");}
};
// Query: handle16,flags16,disabled32,completion32,count32,objectIDs[9]64,
// groupHandles[8]16,referenceCounts[9]32 => total140 bytes.
int main(int argc,char**argv){try{
    if(argc!=5)throw std::runtime_error("datacache farm_gate.mdl queries output required");const auto mdl=read(argv[2]),q=read(argv[3]);if(mdl.size()<400||std::memcmp(mdl.data(),"IDST",4)||at<uint32_t>(mdl.data(),336)!=0)throw std::runtime_error("pinned no-include farm gate header");if(q.empty()||q.size()%140)throw std::runtime_error("cache query extent");std::ofstream out(argv[4],std::ios::binary);if(!out)throw std::runtime_error("cache output");
#ifdef NATIVE_STUDIO_VCOLLIDE_READY
    static_assert(sizeof(void*)==4,"native x86");AddVectoredExceptionHandler(1,fault);NativePrivateMdlCacheFixture image(argv[1]);uint8_t*base=image.base;std::array<uint8_t,64>disabledCvar{},logCvar{};set(base,0x3e834,uintptr_t(disabledCvar.data()));set(base,0x3e8ec,uintptr_t(logCvar.data()));std::array<uint8_t,192>cache{};set(cache.data(),0,uint32_t(0x1002acbc));std::array<uint8_t,9*16>entries{};set(cache.data(),0x28,uintptr_t(entries.data()));std::array<std::array<uint8_t,144>,9>records{};std::array<std::array<uint8_t,24>,9>objects{};std::array<uint8_t,0xa8>vm{};std::array<uint8_t,8*0x90>groups{};std::vector<uint8_t>header=mdl;set(vm.data(),0x58,uintptr_t(groups.data()));
    for(size_t i=0;i<9;++i){set(entries.data(),i*16+12,uintptr_t(records[i].data()));set(records[i].data(),0x68,uintptr_t(header.data()));}
    image.seal();using Complete=bool(__thiscall*)(void*,uint16_t,int,int,const void*,int,bool);using Query=void*(__thiscall*)(void*,uint16_t);const auto complete=reinterpret_cast<Complete>(base+0xeb80);const auto query=reinterpret_cast<Query>(base+0xceb0);
#else
    static_assert(sizeof(void*)==8,"owned x64");
#endif
    size_t cases=0;
    for(size_t p=0;p<q.size();p+=140){const auto row=q.data()+p;const uint16_t handle=at<uint16_t>(row,0),flags=at<uint16_t>(row,2);const bool disabled=at<uint32_t>(row,4)!=0,completion=at<uint32_t>(row,8)!=0;const uint32_t count=at<uint32_t>(row,12);if((handle!=0&&handle!=0xffff)||count>8||(!(flags&2)&&!completion&&!disabled&&handle!=0xffff))throw std::runtime_error("unready/invalid native cache query");Services services;for(size_t i=0;i<9;++i){services.objects[i]=at<uint64_t>(row,16+i*8);services.refs[i]=at<uint32_t>(row,104+i*4);}for(size_t i=0;i<count;++i){uint16_t h=at<uint16_t>(row,88+i*2);if(h<1||h>8)throw std::runtime_error("group handle range");services.groups.push_back(h);}SourceStudioVCollideRecordCandidate record{flags,services.objects[0]};uint32_t completedReturn=2;std::array<uint64_t,2>answers{};
#ifdef NATIVE_STUDIO_VCOLLIDE_READY
        activeCase=cases;set(disabledCvar.data(),0x30,uint32_t(disabled));set(records[0].data(),0x20,flags);set(header.data(),336,uint32_t(count!=0));set(records[0].data(),0x24,uintptr_t(vm.data()));set(vm.data(),0x64,count);for(size_t i=0;i<count;++i)set(groups.data(),i*0x90,uint32_t(services.groups[i]));for(size_t i=0;i<9;++i){set(records[i].data(),0x0c,services.objects[i]?uintptr_t(objects[i].data()):uintptr_t(0));set(objects[i].data(),4,services.refs[i]);}const auto beforeHeader=header;const auto beforeGroups=groups;const auto beforeEntries=entries;
        const auto beforeRecords=records;const auto beforeObjects=objects;const auto beforeCache=cache;const auto beforeVm=vm;
        if(completion)completedReturn=complete(cache.data(),0,2,0,nullptr,0,false);
        for(size_t call=0;call<2;++call){void*value=query(cache.data(),handle);if(value){size_t found=9;for(size_t i=0;i<9;++i)if(value==objects[i].data()+8){found=i;break;}if(found==9)throw std::runtime_error("unrecognized native collision identity");answers[call]=services.objects[found];}}
        record.flags=at<uint16_t>(records[0].data(),0x20);const auto pointer=at<uintptr_t>(records[0].data(),0x0c);record.collisionObjectId=0;if(pointer){size_t found=9;for(size_t i=0;i<9;++i)if(pointer==uintptr_t(objects[i].data())){found=i;break;}if(found==9)throw std::runtime_error("native object assignment");record.collisionObjectId=services.objects[found];}for(size_t i=0;i<9;++i)services.refs[i]=at<uint32_t>(objects[i].data(),4);if(header!=beforeHeader||groups!=beforeGroups||entries!=beforeEntries||cache!=beforeCache||vm!=beforeVm)throw std::runtime_error("native immutable cache inputs changed");
        for(size_t n=0;n<9;++n){for(size_t k=0;k<records[n].size();++k)if(!(n==0&&((k>=12&&k<16)||(k>=32&&k<34)))&&records[n][k]!=beforeRecords[n][k])throw std::runtime_error("native write outside record fields");for(size_t k=0;k<objects[n].size();++k)if(!(k>=4&&k<8)&&objects[n][k]!=beforeObjects[n][k])throw std::runtime_error("native write outside reference count");}
#else
        if(completion)completedReturn=sourceStudioVCollideFailedCompletionCandidate(record);for(auto&answer:answers)answer=sourceStudioVCollideReadyQueryCandidate(record,handle,disabled,services);
#endif
        put(out,completedReturn);put(out,record.flags);put(out,uint16_t(0));put(out,record.collisionObjectId);put(out,answers);put(out,services.refs);++cases;
    }
    out.flush();if(!out)throw std::runtime_error("cache output failure");
#ifdef NATIVE_STUDIO_VCOLLIDE_READY
    image.verify();
#endif
    std::cout<<"PASS ready-cache queries="<<cases<<" repeated twice each\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
