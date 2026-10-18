#include "source_world_light_entity_filter_candidate.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
struct Fixture {uint32_t entity=0,excluded=0,handle=0,mask=0,mode=1,context=0;};
struct Result {uint32_t classified=0,rawClassified=0,index=UINT32_MAX,accepted=0,mode=0,calls=0;std::array<uint32_t,3>queried{};};
static_assert(sizeof(Fixture)==24&&sizeof(Result)==36,"filter test wire");
static const Fixture*active=nullptr;static Result events;static size_t activeCase=0;
static void validate(const Fixture&f){if(f.entity>3||f.excluded>3||(f.mode!=1&&f.mode!=3))throw std::runtime_error("entity filter fixture range");}
#ifdef NATIVE_WORLD_LIGHT_ENTITY_FILTER
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
static std::array<uint32_t,3>entities{};
static const uint32_t* __fastcall referenceHandle(void*self,void*){
    uint32_t label=0;for(uint32_t i=0;i<3;++i)if(self==&entities[i])label=i+1;
    if(!label||label!=active->entity||events.calls>=3)throw std::runtime_error("unexpected native entity handle query");
    events.queried[events.calls++]=label;return &active->handle;
}
static void* identity(uint32_t label){return label?&entities.at(label-1):nullptr;}
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"filter fault case="<<activeCase<<" eip="<<std::hex<<p->ContextRecord->Eip<<std::endl;ExitProcess(90);}
#else
// Same low32 values for labels1/2 intentionally catch pointer truncation.
static uintptr_t identity(uint32_t label){static constexpr uintptr_t values[]={0,0x123400001234ull,0x567800001234ull,0x123400005678ull};return values[label];}
struct Services {
    uint32_t referenceHandle(uintptr_t entity){
        if(entity!=identity(active->entity)||!entity||events.calls>=3)throw std::runtime_error("unexpected owned entity handle query");
        events.queried[events.calls++]=active->entity;return active->handle;
    }
};
#endif
int main(int argc,char**argv){try{
    if(argc!=4)throw std::runtime_error("engine input output required");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("filter IO");
#ifdef NATIVE_WORLD_LIGHT_ENTITY_FILTER
    static_assert(sizeof(void*)==4,"native x86");AddVectoredExceptionHandler(1,fault);NativePrivateWorldImageFixture image(argv[1]);
    // Manager getter and all its original virtuals remain intact. Only the
    // entity GetRefEHandle virtual is supplied by the fixture host.
    std::array<uint32_t,3>entityTable{};entityTable[2]=reinterpret_cast<uint32_t>(&referenceHandle);entities.fill(reinterpret_cast<uint32_t>(entityTable.data()));const auto beforeEntities=entities;
    const auto beforeManager=std::array<uint32_t,3>{*reinterpret_cast<uint32_t*>(image.base+0x40b598),*reinterpret_cast<uint32_t*>(image.base+0x40b59c),*reinterpret_cast<uint32_t*>(image.base+0x40b5a0)};
    if(beforeManager[0]!=0x1035fcdc||*reinterpret_cast<uint32_t*>(image.base+0x35fcdc+11*4)!=0x101c4700)throw std::runtime_error("original static manager binding");
    image.seal();using EntityPredicate=bool(__thiscall*)(void*,void*);using RawPredicate=bool(__thiscall*)(void*,uint32_t);using Index=uint32_t(__thiscall*)(void*,void*);using Filter=bool(__thiscall*)(const void*,void*,uint32_t);using Type=uint32_t(__thiscall*)(const void*);
    void*manager=image.base+0x40b598;const auto managerTable=*reinterpret_cast<uint32_t*>(manager);
    const auto classify=reinterpret_cast<EntityPredicate>(*reinterpret_cast<uint32_t*>(managerTable+44));const auto classifyRaw=reinterpret_cast<RawPredicate>(*reinterpret_cast<uint32_t*>(managerTable+40));const auto index=reinterpret_cast<Index>(*reinterpret_cast<uint32_t*>(managerTable+48));
#else
    static_assert(sizeof(void*)==8,"owned AMD64");Services services;
#endif
    Fixture f;size_t cases=0;while(in.read(reinterpret_cast<char*>(&f),sizeof(f))){validate(f);active=&f;activeCase=cases;events={};const auto before=f;
#ifdef NATIVE_WORLD_LIGHT_ENTITY_FILTER
        std::array<uint32_t,5>filter{0xa7b8c9da,f.mode==3?0x1035bb48u:0x1031dee4u,reinterpret_cast<uint32_t>(identity(f.excluded)),f.context,0xa7b8c9da};const auto beforeFilter=filter;
        events.classified=classify(manager,identity(f.entity));events.rawClassified=classifyRaw(manager,f.handle);if(f.entity)events.index=index(manager,identity(f.entity));
        const auto table=filter[1];events.mode=reinterpret_cast<Type>(*reinterpret_cast<uint32_t*>(table+4))(filter.data()+1);events.accepted=reinterpret_cast<Filter>(*reinterpret_cast<uint32_t*>(table))(filter.data()+1,identity(f.entity),f.mask);
        if(filter!=beforeFilter||entities!=beforeEntities||std::memcmp(image.base+0x40b598,beforeManager.data(),12))throw std::runtime_error("native filter/manager mutation");
#else
        const SourceWorldLightFilterCandidate filter{SourceWorldLightTraceModeCandidate(f.mode),identity(f.excluded)};
        events.classified=sourceStaticPropEntityCandidate(identity(f.entity),services);events.rawClassified=sourceStaticPropReferenceHandleCandidate(f.handle);if(f.entity)events.index=sourceStaticPropEntityIndexCandidate(identity(f.entity),services);events.mode=uint32_t(filter.mode);events.accepted=sourceWorldLightShouldHitEntityCandidate(filter,identity(f.entity),f.mask,services);
#endif
        if(std::memcmp(&f,&before,sizeof(f)))throw std::runtime_error("filter fixture changed");out.write(reinterpret_cast<const char*>(&events),sizeof(events));++cases;
    }if(!in.eof()||in.gcount()!=0||!cases)throw std::runtime_error("filter input extent");out.flush();if(!out)throw std::runtime_error("filter output");
#ifdef NATIVE_WORLD_LIGHT_ENTITY_FILTER
    image.verify();VirtualFree(image.base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS entity filter cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
