#include "source_particle_trace_filter_candidate.h"

#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>

struct Query {
    uint32_t mask;
    int32_t particleGroup, entityGroup, model, solid, movement;
    uint32_t transparent, entityPresent, exclusion, entityAllows, rulesAllow;
};
static_assert(sizeof(Query)==44,"fixture record size");
struct GroupQuery {
    int32_t first, second;
    uint32_t mask;
};
static_assert(sizeof(GroupQuery)==12,"group record size");
static const Query* active;
static uint32_t calls;

static bool recordEntityRule(int32_t group,uint32_t mask){
    if(calls!=0||group!=active->particleGroup||mask!=active->mask)
        throw std::runtime_error("entity rule call contract");
    calls=1;
    return active->entityAllows!=0;
}
static bool recordGroupRule(int32_t group,int32_t entityGroup){
    if(calls!=1||group!=active->particleGroup||entityGroup!=active->entityGroup)
        throw std::runtime_error("game rules call contract");
    calls=3;
    return active->rulesAllow!=0;
}

#ifdef NATIVE_PARTICLE_TRACE_FILTER
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"

template<class T>
static void store(void* object,size_t offset,T value){
    std::memcpy(static_cast<uint8_t*>(object)+offset,&value,sizeof(value));
}
alignas(4) static uint8_t entity[0x454],ignored[0x454];
static std::array<uintptr_t,9> entityHandle{},ignoredHandle{};
static uint16_t renderInfo[1]{9};
static void* __fastcall handleEntity(void* self,void*){
    if(self==entityHandle.data())return active->entityPresent?entity:nullptr;
    if(self==ignoredHandle.data())return active->exclusion==5?nullptr:ignored;
    throw std::runtime_error("unexpected fixture handle");
}
static int32_t __fastcall solidType(void*,void*){return active->solid;}
static const uint16_t* __fastcall renderHandle(void*,void*){return renderInfo;}
static int32_t __fastcall modelType(void*,void*,void*){return active->model;}
static void* __fastcall model(void*,void*){return renderInfo;}
static int32_t __fastcall modelTransparency(void*,void*,uint32_t id){
    if(id!=9)throw std::runtime_error("unexpected fixture render handle");
    return active->transparent?1:0;
}
static bool __fastcall entityRule(void* self,void*,int32_t group,uint32_t mask){
    if(self!=entity)throw std::runtime_error("unexpected fixture entity");
    return recordEntityRule(group,mask);
}
static bool __fastcall groupRule(void*,void*,int32_t group,int32_t targetGroup){
    return recordGroupRule(group,targetGroup);
}
static LONG CALLBACK fault(EXCEPTION_POINTERS* error){
    if(error->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;
    std::cerr<<"native fixture fault at "<<std::hex<<error->ContextRecord->Eip<<'\n';
    ExitProcess(90);
}
#endif

int main(int argc,char** argv){
    try{
        const bool realRules=argc==5&&std::strcmp(argv[4],"--sfm-rules")==0;
        const bool groupsOnly=argc==5&&std::strcmp(argv[4],"--groups-only")==0;
        if(argc!=4&&!realRules&&!groupsOnly)throw std::runtime_error("client query and result files required");
        std::ifstream in(argv[2],std::ios::binary);
        std::ofstream out(argv[3],std::ios::binary);
        if(!in||!out)throw std::runtime_error("fixture streams");
#ifdef NATIVE_PARTICLE_TRACE_FILTER
        static_assert(sizeof(void*)==4,"native reference is x86");
        AddVectoredExceptionHandler(1,fault);
        NativePrivateWorldImageFixture image(argv[1],0x6c28cc);
        std::array<uintptr_t,161> baseVtable{};
        std::array<uintptr_t,9> renderVtable{};
        std::array<uintptr_t,10> modelVtable{};
        std::array<uintptr_t,30> opacityVtable{};
        std::array<uintptr_t,29> rulesVtable{};
        baseVtable[0x134/4]=reinterpret_cast<uintptr_t>(&solidType);
        baseVtable[0x280/4]=realRules?
            *reinterpret_cast<const uintptr_t*>(image.base+0x4deec4+0x280):
            reinterpret_cast<uintptr_t>(&entityRule);
        renderVtable[0x1c/4]=reinterpret_cast<uintptr_t>(&renderHandle);
        renderVtable[0x20/4]=reinterpret_cast<uintptr_t>(&model);
        modelVtable[0x24/4]=reinterpret_cast<uintptr_t>(&modelType);
        opacityVtable[0x74/4]=reinterpret_cast<uintptr_t>(&modelTransparency);
        rulesVtable[0x70/4]=reinterpret_cast<uintptr_t>(&groupRule);
        uintptr_t modelObject=reinterpret_cast<uintptr_t>(modelVtable.data());
        uintptr_t opacityObject=reinterpret_cast<uintptr_t>(opacityVtable.data());
        uintptr_t rulesObject=realRules?reinterpret_cast<uintptr_t>(image.base+0x53a4b4):
            reinterpret_cast<uintptr_t>(rulesVtable.data());
        store(image.base,0x6c28c8,&modelObject);
        store(image.base,0x63d3a8,&opacityObject);
        store(image.base,0x6c2438,&rulesObject);
        std::array<uint32_t,13> owners{};
        owners[5]=reinterpret_cast<uint32_t>(entity);
        owners[6]=37;
        owners[9]=reinterpret_cast<uint32_t>(ignored);
        owners[10]=38;
        store(image.base,0x63d340,owners.data());
        for(auto* raw:{entity,ignored}){
            store(raw,0,baseVtable.data());
            store(raw,4,renderVtable.data());
        }
        const auto getBase=reinterpret_cast<uintptr_t>(&handleEntity);
        entityHandle[0]=reinterpret_cast<uintptr_t>(&entityHandle[1]);
        ignoredHandle[0]=reinterpret_cast<uintptr_t>(&ignoredHandle[1]);
        entityHandle[1+0x1c/4]=getBase;
        ignoredHandle[1+0x1c/4]=getBase;
        using Construct=void*(__thiscall*)(void*,void*,int32_t,void*);
        using Filter=bool(__thiscall*)(void*,void*,uint32_t);
        const auto construct=reinterpret_cast<Construct>(image.base+0x19c8e0);
        const auto filter=reinterpret_cast<Filter>(image.base+0x19c910);
        image.seal();
#else
        static_assert(sizeof(void*)==8,"owned candidate is x64");
        constexpr uintptr_t ownedEntity=0xfedc00001234ull;
#endif
        if(groupsOnly){
            GroupQuery query{};
            size_t count=0;
            while(in.read(reinterpret_cast<char*>(&query),sizeof(query))){
#ifdef NATIVE_PARTICLE_TRACE_FILTER
                using GroupRule=bool(__thiscall*)(void*,int32_t,int32_t);
                using EntityRule=bool(__thiscall*)(void*,int32_t,uint32_t);
                uintptr_t baseRulesVtable=reinterpret_cast<uintptr_t>(image.base+0x4f1324);
                uintptr_t tfRulesVtable=reinterpret_cast<uintptr_t>(image.base+0x53a4b4);
                const auto base=reinterpret_cast<GroupRule>(*reinterpret_cast<const uintptr_t*>(baseRulesVtable+0x70));
                const auto tf=reinterpret_cast<GroupRule>(*reinterpret_cast<const uintptr_t*>(tfRulesVtable+0x70));
                const auto baseEntity=reinterpret_cast<EntityRule>(
                    *reinterpret_cast<const uintptr_t*>(image.base+0x4deec4+0x280));
                store(entity,0x450,query.second);
                const std::array<uint8_t,3> response{
                    uint8_t(base(&baseRulesVtable,query.first,query.second)),
                    uint8_t(tf(&tfRulesVtable,query.first,query.second)),
                    uint8_t(baseEntity(entity,query.first,query.mask))};
#else
                const std::array<uint8_t,3> response{
                    uint8_t(sourceSfmBaseCollisionGroupsCandidate(query.first,query.second)),
                    uint8_t(sourceSfmTfCollisionGroupsCandidate(query.first,query.second)),
                    uint8_t(sourceSfmBaseEntityCollisionCandidate(query.second,query.mask))};
#endif
                out.write(reinterpret_cast<const char*>(response.data()),sizeof(response));
                ++count;
            }
            if(!in.eof()||in.gcount()||!out||!count)throw std::runtime_error("collision group fixture extent");
#ifdef NATIVE_PARTICLE_TRACE_FILTER
            image.verify();
            VirtualFree(image.base,0,MEM_RELEASE);
#endif
            std::cout<<"Collision rule cases="<<count<<'\n';
            return 0;
        }
        Query query{};
        size_t count=0;
        while(in.read(reinterpret_cast<char*>(&query),sizeof(query))){
            active=&query;
            calls=0;
            bool passed;
#ifdef NATIVE_PARTICLE_TRACE_FILTER
            store(entity,0x138,query.exclusion==2?0x00260002u:
                               query.exclusion==4?0x00250002u:UINT32_MAX);
            store(ignored,0x138,query.exclusion==3?0x00250001u:UINT32_MAX);
            store(entity,0x248,static_cast<uint8_t>(query.movement));
            store(entity,0x450,query.entityGroup);
            std::array<uintptr_t,4> object{};
            auto* excluded=query.exclusion==0?nullptr:
                           query.exclusion==1?entityHandle.data():ignoredHandle.data();
            if(construct(object.data(),excluded,query.particleGroup,nullptr)!=object.data())
                throw std::runtime_error("native trace filter constructor");
            passed=filter(object.data(),entityHandle.data(),query.mask);
#else
            const uintptr_t entityHandleId=0x100001234ull,otherHandleId=0x200001234ull;
            const uintptr_t otherEntity=0x123400001234ull;
            const SourceParticleTraceEntityCandidate candidate{entityHandleId,
                query.entityPresent?ownedEntity:0,query.exclusion==2?otherEntity:0,
                query.model,query.solid,query.movement,query.entityGroup,query.transparent!=0};
            SourceParticleTraceEntityCandidate excluded{};
            if(query.exclusion){
                excluded.handle=query.exclusion==1?entityHandleId:otherHandleId;
                excluded.baseEntity=query.exclusion==1?candidate.baseEntity:
                                    query.exclusion==5?0:otherEntity;
                excluded.owner=query.exclusion==3?ownedEntity:0;
            }
            if(realRules){
                passed=sourceSfmParticleTraceShouldHitBaseEntityCandidate(candidate,excluded,query.mask,
                                                                        query.particleGroup);
            }else passed=sourceParticleTraceShouldHitEntityCandidate(candidate,excluded,query.mask,
                    query.particleGroup,[&](uintptr_t id,int32_t group,uint32_t mask){
                        if(id!=ownedEntity)throw std::runtime_error("owned entity identity");
                        return recordEntityRule(group,mask);
                    },recordGroupRule);
#endif
            const std::array<uint32_t,2> response{uint32_t(passed),calls};
            out.write(reinterpret_cast<const char*>(response.data()),sizeof(response));
            ++count;
        }
        if(!in.eof()||in.gcount()||!out||!count)throw std::runtime_error("fixture result or input extent");
#ifdef NATIVE_PARTICLE_TRACE_FILTER
        image.verify();
        VirtualFree(image.base,0,MEM_RELEASE);
#endif
        std::cout<<"Particle client filter cases="<<count<<'\n';
        return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
