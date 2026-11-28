#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "source_engine_collideable_clip_dispatch_candidate.h"
#include "physics_search_test_fixture.h"
struct ClipLeafFixture {uint32_t returned=0;SourcePhysicsSearchTraceCandidate trace;};
struct ClipDispatchFixture {
    std::array<uint8_t,80> rawRay{};
    uint32_t modelType=UINT32_MAX,solidType=0,flagsFirst=0,flagsLater=0,contents=0,surface=0,root=0,mask=0;
    uint64_t annotationEntity=0;uint32_t annotationHitBox=0;
    std::array<ClipLeafFixture,6> leaves;
};
struct ClipDispatchEventFixture {uint32_t kind=0;std::array<uint8_t,80> ray{};};
static const ClipDispatchFixture* active=nullptr;
static std::vector<ClipDispatchEventFixture> events;
static uint32_t flagCalls=0;
static size_t activeCase=0;
static void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
static bool readQuery(std::istream& in,ClipDispatchFixture& query){
    if(in.peek()==std::char_traits<char>::eof())return false;
    query={};supportGet(in,query.rawRay);
    for(auto* value:{&query.modelType,&query.solidType,&query.flagsFirst,&query.flagsLater,&query.contents,&query.surface,&query.root,&query.mask})supportGet(in,*value);
    supportGet(in,query.annotationEntity);supportGet(in,query.annotationHitBox);
    require(query.surface<=65535&&query.rawRay[68]<=1&&query.rawRay[69]<=1,"clip dispatch input controls");
    for(unsigned offset:{0u,16u,32u,48u})for(unsigned axis=0;axis<3;++axis){float value;std::memcpy(&value,query.rawRay.data()+offset+axis*4,4);require(std::isfinite(value),"nonfinite dispatch ray");}
    for(auto& leaf:query.leaves){supportGet(in,leaf.returned);require(leaf.returned<=1,"clip leaf boolean");searchReadTrace(in,leaf.trace);for(const auto& values:{leaf.trace.start,leaf.trace.end,leaf.trace.plane.normal})for(float value:values)require(std::isfinite(value),"nonfinite dispatch trace");for(float value:{leaf.trace.plane.distance,leaf.trace.fraction,leaf.trace.fractionLeftSolid})require(std::isfinite(value),"nonfinite dispatch trace scalar");}
    return true;
}
#ifndef NATIVE_ENGINE_COLLIDEABLE_CLIP_DISPATCH
static SourceEngineCollideableClipRayCandidate semanticRay(const std::array<uint8_t,80>& bytes){SourceEngineCollideableClipRayCandidate ray;std::memcpy(ray.ray.center.data(),bytes.data(),12);std::memcpy(ray.ray.delta.data(),bytes.data()+16,12);std::memcpy(ray.ray.offset.data(),bytes.data()+32,12);std::memcpy(ray.ray.extents.data(),bytes.data()+48,12);uint32_t transform;std::memcpy(&transform,bytes.data()+64,4);ray.transformId=transform;ray.ray.isRay=bytes[68]!=0;ray.ray.swept=bytes[69]!=0;return ray;}
static std::array<uint8_t,80> observedRay(const SourceEngineCollideableClipRayCandidate& ray){auto bytes=active->rawRay;std::memcpy(bytes.data(),ray.ray.center.data(),12);std::memcpy(bytes.data()+16,ray.ray.delta.data(),12);std::memcpy(bytes.data()+32,ray.ray.offset.data(),12);std::memcpy(bytes.data()+48,ray.ray.extents.data(),12);require(ray.transformId<=UINT32_MAX,"test transform wire range");const auto transform=uint32_t(ray.transformId);std::memcpy(bytes.data()+64,&transform,4);bytes[68]=uint8_t(ray.ray.isRay);bytes[69]=uint8_t(ray.ray.swept);return bytes;}
#endif
static void writeOutput(std::ostream& out,const SourcePhysicsSearchTraceCandidate& trace){std::ostringstream bytes(std::ios::binary);searchWriteTrace(bytes,trace);supportPut(bytes,uint32_t(events.size()));for(const auto& event:events){supportPut(bytes,event.kind);supportPut(bytes,event.ray);}const auto data=bytes.str();supportPut(out,uint32_t(data.size()));out<<data;}

#ifdef NATIVE_ENGINE_COLLIDEABLE_CLIP_DISPATCH
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"
static uint8_t* nativeRay=nullptr;
static uint8_t* nativeModel=nullptr;
static uint8_t* nativeStudio=nullptr;
static void* nativeCollideable=nullptr;
static void* nativeEngine=nullptr;
static uint32_t defaultSurface=0;
static std::vector<uint64_t> surfaceIds,entityIds;
template<class T>static T readAt(const uint8_t* value,size_t offset){T result;std::memcpy(&result,value+offset,sizeof(result));return result;}
template<class T>static void writeAt(uint8_t* value,size_t offset,T result){std::memcpy(value+offset,&result,sizeof(result));}
static void nativeEvent(uint32_t kind){ClipDispatchEventFixture event;event.kind=kind;std::memcpy(event.ray.data(),nativeRay,80);events.push_back(event);}
static uint32_t encodeIdentity(std::vector<uint64_t>& ids,uint64_t value,uint32_t base){if(!value)return 0;for(size_t i=0;i<ids.size();++i)if(ids[i]==value)return base+uint32_t(i);ids.push_back(value);return base+uint32_t(ids.size()-1);}
static uint64_t decodeIdentity(const std::vector<uint64_t>& ids,uint32_t value,uint32_t base){if(!value)return 0;require(value>=base&&uint64_t(value-base)<ids.size(),"native opaque identity");return ids[value-base];}
static uint32_t encodeSurface(uint64_t value){return !value?defaultSurface:value==UINT64_MAX?0x103545f8u:encodeIdentity(surfaceIds,value,0x20000000u);}
static uint64_t decodeSurface(uint32_t value){return value==defaultSurface?0:value==0x103545f8u?UINT64_MAX:decodeIdentity(surfaceIds,value,0x20000000u);}
static void nativeWriteTrace(uint8_t* output,const SourcePhysicsSearchTraceCandidate& trace){std::memcpy(output,trace.start.data(),12);std::memcpy(output+12,trace.end.data(),12);std::memcpy(output+24,trace.plane.normal.data(),12);writeAt(output,36,trace.plane.distance);std::memcpy(output+40,trace.planeTags.data(),4);writeAt(output,44,trace.fraction);writeAt(output,48,trace.contents);writeAt(output,52,trace.displacementFlags);output[54]=trace.allSolid;output[55]=trace.startSolid;writeAt(output,56,trace.fractionLeftSolid);writeAt(output,60,encodeSurface(trace.surfaceId));writeAt(output,64,trace.surfaceMetadata);writeAt(output,68,trace.hitGroup);writeAt(output,72,trace.physicsBone);writeAt(output,74,trace.worldSurface);writeAt(output,76,encodeIdentity(entityIds,trace.entityId,0x30000000u));writeAt(output,80,trace.hitBox);}
static SourcePhysicsSearchTraceCandidate nativeReadTrace(const uint8_t* value){SourcePhysicsSearchTraceCandidate trace;std::memcpy(trace.start.data(),value,12);std::memcpy(trace.end.data(),value+12,12);std::memcpy(trace.plane.normal.data(),value+24,12);trace.plane.distance=readAt<float>(value,36);std::memcpy(trace.planeTags.data(),value+40,4);trace.fraction=readAt<float>(value,44);trace.contents=readAt<uint32_t>(value,48);trace.displacementFlags=readAt<uint16_t>(value,52);trace.allSolid=value[54];trace.startSolid=value[55];trace.fractionLeftSolid=readAt<float>(value,56);trace.surfaceId=decodeSurface(readAt<uint32_t>(value,60));trace.surfaceMetadata=readAt<uint32_t>(value,64);trace.hitGroup=readAt<uint32_t>(value,68);trace.physicsBone=readAt<uint16_t>(value,72);trace.worldSurface=readAt<uint16_t>(value,74);trace.entityId=decodeIdentity(entityIds,readAt<uint32_t>(value,76),0x30000000u);trace.hitBox=readAt<uint32_t>(value,80);return trace;}
static const uint8_t* __fastcall getModel(void* self,void*){require(self==nativeCollideable,"native model receiver");nativeEvent(1);return active->modelType==UINT32_MAX?nullptr:nativeModel;}
static const uint8_t* __fastcall getStudio(void*,void*,const void* model){require(model==nativeModel,"native studio model");nativeEvent(2);return nativeStudio;}
static uint32_t __fastcall getFlags(void* self,void*){require(self==nativeCollideable,"native flags receiver");nativeEvent(3);return flagCalls++?active->flagsLater:active->flagsFirst;}
static uint32_t __fastcall getRoot(void* self,void*){require(self==nativeCollideable,"native root receiver");nativeEvent(4);return active->root;}
static uint32_t __fastcall getSolid(void* self,void*){require(self==nativeCollideable,"native solid receiver");nativeEvent(5);return active->solidType;}
static bool __fastcall custom(void* self,void*,uint8_t* ray,uint32_t mask,uint8_t* output){require(self==nativeCollideable&&ray==nativeRay&&mask==active->mask,"native custom arguments");nativeEvent(6);nativeWriteTrace(output,active->leaves[0].trace);return active->leaves[0].returned!=0;}
static void __fastcall annotate(void* self,void*,void* collideable,uint8_t* output){require(self==nativeEngine&&collideable==nativeCollideable,"native annotation arguments");nativeEvent(12);writeAt(output,76,encodeIdentity(entityIds,active->annotationEntity,0x30000000u));writeAt(output,80,active->annotationHitBox);}
static void boundary(unsigned,CONTEXT& context){
    const auto* args=reinterpret_cast<const uint32_t*>(context.Esp);unsigned leaf=0;uint32_t returnAddress=0;
    switch(context.Eip){case 0x1016e170:leaf=1;returnAddress=0x1016f44a;break;case 0x1016d6f0:leaf=2;returnAddress=0x1016f46d;break;case 0x1016d740:leaf=3;returnAddress=0x1016f480;break;case 0x1016f140:leaf=4;returnAddress=0x1016f4a7;break;case 0x1016e2d0:leaf=5;returnAddress=0x1016f4c2;break;default:throw std::runtime_error("unexpected clip boundary");}
    require(context.Ecx==uintptr_t(nativeEngine)&&args[0]==returnAddress&&args[1]==uintptr_t(nativeRay)&&args[2]==active->mask&&args[3]==uintptr_t(nativeCollideable),"native clip leaf arguments");
    if(leaf==1)require(args[4]==(active->modelType==3?uintptr_t(nativeStudio):0),"native physics studio argument");
    nativeEvent(6+leaf);nativeWriteTrace(reinterpret_cast<uint8_t*>(args[leaf==1?5:4]),active->leaves[leaf].trace);
    PrivateX86BoundaryObserverFixture::returnCdecl(context,active->leaves[leaf].returned);context.Esp+=leaf==1?20:16;
}
static LONG CALLBACK fault(EXCEPTION_POINTERS* info){if(info->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"clip dispatch fault case="<<activeCase<<" eip="<<std::hex<<info->ContextRecord->Eip<<'\n';ExitProcess(90);}
#else
struct Services {
    void record(uint32_t kind,const SourceEngineCollideableClipRayCandidate& ray){events.push_back({kind,observedRay(ray)});}
    SourceEngineCollideableClipModelCandidate model(const SourceEngineCollideableClipRayCandidate& ray){record(1,ray);return {active->modelType!=UINT32_MAX,active->modelType};}
    SourceEngineCollideableClipStudioCandidate studio(const SourceEngineCollideableClipRayCandidate& ray){record(2,ray);return {active->contents,uint16_t(active->surface)};}
    uint32_t flags(const SourceEngineCollideableClipRayCandidate& ray){record(3,ray);return flagCalls++?active->flagsLater:active->flagsFirst;}
    uint64_t rootTransform(const SourceEngineCollideableClipRayCandidate& ray){record(4,ray);return active->root;}
    uint32_t solidType(const SourceEngineCollideableClipRayCandidate& ray){record(5,ray);return active->solidType;}
    bool leaf(unsigned index,const SourceEngineCollideableClipRayCandidate& ray,uint32_t mask,SourcePhysicsSearchTraceCandidate& trace){require(mask==active->mask,"owned leaf mask");record(6+index,ray);trace=active->leaves[index].trace;return active->leaves[index].returned!=0;}
    bool custom(const SourceEngineCollideableClipRayCandidate& ray,uint32_t mask,SourcePhysicsSearchTraceCandidate& trace){return leaf(0,ray,mask,trace);}
    bool physics(const SourceEngineCollideableClipRayCandidate& ray,uint32_t mask,SourcePhysicsSearchTraceCandidate& trace){return leaf(1,ray,mask,trace);}
    bool brush(const SourceEngineCollideableClipRayCandidate& ray,uint32_t mask,SourcePhysicsSearchTraceCandidate& trace){return leaf(2,ray,mask,trace);}
    bool obb(const SourceEngineCollideableClipRayCandidate& ray,uint32_t mask,SourcePhysicsSearchTraceCandidate& trace){return leaf(3,ray,mask,trace);}
    bool hitbox(const SourceEngineCollideableClipRayCandidate& ray,uint32_t mask,SourcePhysicsSearchTraceCandidate& trace){return leaf(4,ray,mask,trace);}
    bool bounds(const SourceEngineCollideableClipRayCandidate& ray,uint32_t mask,SourcePhysicsSearchTraceCandidate& trace){return leaf(5,ray,mask,trace);}
    void annotate(const SourceEngineCollideableClipRayCandidate& ray,SourcePhysicsSearchTraceCandidate& trace){record(12,ray);trace.entityId=active->annotationEntity;trace.hitBox=active->annotationHitBox;}
};
#endif
int main(int argc,char** argv){try{
    require(argc==4,"expected engine or -, input, output");std::ifstream input(argv[2],std::ios::binary);std::ofstream output(argv[3],std::ios::binary);require(bool(input)&&bool(output),"clip dispatch streams");
#ifdef NATIVE_ENGINE_COLLIDEABLE_CLIP_DISPATCH
    static_assert(sizeof(void*)==4,"original x86 fixture");AddVectoredExceptionHandler(1,fault);NativePrivateWorldImageFixture image(argv[1]);
    defaultSurface=*reinterpret_cast<uint32_t*>(image.base+0x402fe8);require(defaultSurface==0x1035067cu&&*reinterpret_cast<uint32_t*>(image.base+0x402fec)==0,"native default trace surface");
    std::array<uint8_t,0x114> model{};nativeModel=model.data();std::array<uint8_t,0x190> studio{};nativeStudio=studio.data();
    std::array<uintptr_t,6> modelInfoTable{};modelInfoTable[5]=uintptr_t(&getStudio);uintptr_t modelInfo=uintptr_t(modelInfoTable.data());*reinterpret_cast<uintptr_t*>(image.base+0x401484)=uintptr_t(&modelInfo);
    std::array<uintptr_t,18> collideableTable{};collideableTable[4]=uintptr_t(&custom);collideableTable[7]=uintptr_t(&getModel);collideableTable[11]=uintptr_t(&getSolid);collideableTable[12]=uintptr_t(&getFlags);collideableTable[17]=uintptr_t(&getRoot);uintptr_t collideable=uintptr_t(collideableTable.data());nativeCollideable=&collideable;
    std::array<uintptr_t,27> engineTable{};engineTable[26]=uintptr_t(&annotate);uintptr_t engine=uintptr_t(engineTable.data());nativeEngine=&engine;
    image.seal();PrivateX86BoundaryObserverFixture observer({0x1016e170,0x1016d740,0x1016f140,0x1016e2d0},boundary);using Clip=void(__thiscall*)(void*,void*,uint32_t,void*,void*);const auto clip=reinterpret_cast<Clip>(0x1016f300);
#else
    static_assert(sizeof(void*)==8,"owned AMD64 fixture");Services services;
#endif
    ClipDispatchFixture query;size_t cases=0;
    while(readQuery(input,query)){active=&query;activeCase=cases;flagCalls=0;events.clear();SourcePhysicsSearchTraceCandidate result;
#ifdef NATIVE_ENGINE_COLLIDEABLE_CLIP_DISPATCH
        writeAt(model.data(),0x110,query.modelType);writeAt(studio.data(),0x14c,query.contents);writeAt(studio.data(),0x18c,uint16_t(query.surface));surfaceIds.clear();entityIds.clear();
        alignas(16)auto ray=query.rawRay;nativeRay=ray.data();const auto oldRay=ray;std::array<uint8_t,116> trace;trace.fill(0xa7);
        observer.entries={0x1016e170,0x1016d740,query.modelType==1?0x1016d6f0u:0x1016f140u,0x1016e2d0};observer.configure(true);clip(nativeEngine,ray.data(),query.mask,nativeCollideable,trace.data()+16);observer.configure(false);
        result=nativeReadTrace(trace.data()+16);require(ray==oldRay,"native ray restoration");for(size_t i=0;i<16;++i)require(trace[i]==0xa7&&trace[100+i]==0xa7,"native trace canaries");
#else
        const auto ray=semanticRay(query.rawRay);result=sourceEngineCollideableClipDispatchCandidate(ray,query.mask,services);require(observedRay(ray)==query.rawRay,"owned input ray preservation");
#endif
        writeOutput(output,result);++cases;
    }
    require(cases>0,"no clip dispatch cases");output.flush();require(bool(output),"clip dispatch output");
#ifdef NATIVE_ENGINE_COLLIDEABLE_CLIP_DISPATCH
    image.verify();
#endif
    std::cout<<"PASS collideable clip dispatch cases="<<cases<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
