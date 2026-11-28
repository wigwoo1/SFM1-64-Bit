#include "engine_hull_trace_test_fixture.h"
#include "source_engine_hull_trace_candidate.h"
#include <fstream>
#include <iostream>
#include <iterator>
static const EngineHullInputFixture* active=nullptr;
static EnginePointEventsFixture events;
static std::array<uint8_t,80> forwarded;
static size_t activeCase=0;
static void require(bool ok,const char* s){if(!ok)throw std::runtime_error(s);}
#ifndef NATIVE_ENGINE_HULL_TRACE
static std::array<uint8_t,80> shortened(const SourceServerHullRayCandidate& ray){
 auto bytes=active->rawRay;std::memcpy(bytes.data(),ray.center.data(),12);std::memcpy(bytes.data()+16,ray.delta.data(),12);
 std::memcpy(bytes.data()+32,ray.offset.data(),12);std::memcpy(bytes.data()+48,ray.extents.data(),12);
 bytes[68]=uint8_t(ray.isRay);bytes[69]=uint8_t(ray.swept);return bytes;
}
#endif
#ifdef NATIVE_ENGINE_HULL_TRACE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"
static uint32_t defaultSurface=0;
static std::vector<std::array<uint32_t,2>> entities;
template<class T>static T at(const uint8_t*p,size_t n){T v;std::memcpy(&v,p+n,sizeof(v));return v;}
template<class T>static void put(uint8_t*p,size_t n,T v){std::memcpy(p+n,&v,sizeof(v));}
static SourcePhysicsSearchTraceCandidate readNative(const uint8_t* p){
 SourcePhysicsSearchTraceCandidate t;std::memcpy(t.start.data(),p,12);std::memcpy(t.end.data(),p+12,12);std::memcpy(t.plane.normal.data(),p+24,12);
 t.plane.distance=at<float>(p,36);std::memcpy(t.planeTags.data(),p+40,4);t.fraction=at<float>(p,44);t.contents=at<uint32_t>(p,48);
 t.displacementFlags=at<uint16_t>(p,52);t.allSolid=p[54];t.startSolid=p[55];t.fractionLeftSolid=at<float>(p,56);
 const auto surface=at<uint32_t>(p,60);t.surfaceId=surface==defaultSurface?0:uint64_t(surface)+1;
 t.surfaceMetadata=at<uint32_t>(p,64);t.hitGroup=at<uint32_t>(p,68);t.physicsBone=at<uint16_t>(p,72);t.worldSurface=at<uint16_t>(p,74);t.entityId=at<uint32_t>(p,76);t.hitBox=at<uint32_t>(p,80);return t;
}
static void writeNative(uint8_t*p,const SourcePhysicsSearchTraceCandidate&t){
 require(t.surfaceId<=0x100000000ull&&t.entityId<=UINT32_MAX,"legacy fixture identifier range");
 std::memcpy(p,t.start.data(),12);std::memcpy(p+12,t.end.data(),12);std::memcpy(p+24,t.plane.normal.data(),12);put(p,36,t.plane.distance);std::memcpy(p+40,t.planeTags.data(),4);
 put(p,44,t.fraction);put(p,48,t.contents);put(p,52,t.displacementFlags);p[54]=t.allSolid;p[55]=t.startSolid;put(p,56,t.fractionLeftSolid);
 put(p,60,t.surfaceId?uint32_t(t.surfaceId-1):defaultSurface);put(p,64,t.surfaceMetadata);put(p,68,t.hitGroup);put(p,72,t.physicsBone);put(p,74,t.worldSurface);put(p,76,uint32_t(t.entityId));put(p,80,t.hitBox);
}
static uint32_t label(const void*p){for(size_t i=0;i<entities.size();++i)if(p==&entities[i])return uint32_t(i+1);throw std::runtime_error("unknown native entity");}
static const uint32_t* __fastcall getHandle(void*self,void*){auto i=label(self);events.calls.push_back({2,i});return &entities[i-1][1];}
static uint32_t __fastcall traceType(void*,void*){return active->mode;}
static bool __fastcall predicate(void*,void*,void* entity,uint32_t mask){require(mask==active->mask,"predicate mask");auto id=label(entity);events.calls.push_back({4,id});return id!=active->excluded;}
static uint32_t __fastcall worldIndex(void*,void*){return 1;}
static void __fastcall annotateWorld(void*,void*,uint32_t i,void*){require(i==1,"annotation index");}
static uint32_t __fastcall partitionMask(void*,void*){return 1;}
static void boundary(unsigned index,CONTEXT& c){
 require(index==0&&c.Eip==0x10149eb0,"unexpected boundary");auto* args=reinterpret_cast<const uint32_t*>(c.Esp);
 require(args[0]==0x1016edb3&&args[2]==0&&args[3]==active->mask&&args[4]==1,"world boundary arguments/caller");
 require(std::memcmp(reinterpret_cast<void*>(args[1]),active->rawRay.data(),80)==0,"world received altered full ray");
 ++events.world;writeNative(reinterpret_cast<uint8_t*>(args[5]),active->world);PrivateX86BoundaryObserverFixture::returnCdecl(c);
}
static void __fastcall enumerate(void*,void*,uint32_t mask,const uint8_t*ray,bool coarse,void*collector){
 require(mask==1&&!coarse,"partition args");++events.partition;std::memcpy(forwarded.data(),ray,80);std::memcpy(events.shortDelta.data(),ray+16,12);
 const auto* vt=*reinterpret_cast<const uintptr_t*const*>(collector);using Collect=bool(__thiscall*)(void*,void*);for(auto&e:entities)(void)reinterpret_cast<Collect>(vt[0])(collector,&e);
}
static void __fastcall resolve(void*,void*,void*e,void**collideable,void**other){events.calls.push_back({1,label(e)});*collideable=e;*other=nullptr;}
static void __fastcall clip(void*,void*,const uint8_t*ray,uint32_t mask,void*c,uint8_t*out){auto id=label(c);require(mask==active->mask&&std::memcmp(ray,forwarded.data(),80)==0,"clip full ray");events.calls.push_back({3,id});writeNative(out,active->candidates[id-1].trace);}
static void __cdecl overflow(int,const char*,...){++events.overflows;}
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"engine hull fault case="<<activeCase<<" eip="<<std::hex<<p->ContextRecord->Eip<<'\n';ExitProcess(90);}
#else
static uintptr_t identity(uint32_t n){return 0x123400000000ull+uintptr_t(n)*0x100000000ull;}
static uint32_t label(uintptr_t e){for(size_t i=0;i<active->candidates.size();++i)if(identity(uint32_t(i+1))==e)return uint32_t(i+1);throw std::runtime_error("owned entity identity");}
struct Services {
 SourcePhysicsSearchTraceCandidate traceWorld(const SourceServerHullRayCandidate&ray,uint32_t mask){require(mask==active->mask&&shortened(ray)==active->rawRay,"owned world full ray");++events.world;return active->world;}
 std::vector<uintptr_t>enumerateCandidates(const SourceServerHullRayCandidate&ray){++events.partition;forwarded=shortened(ray);events.shortDelta=ray.delta;std::vector<uintptr_t>list;for(size_t i=0;i<active->candidates.size();++i)list.push_back(identity(uint32_t(i+1)));return list;}
 void candidateOverflow(size_t n){events.overflows+=uint32_t(n);}
 uintptr_t resolveCollideable(uintptr_t e){events.calls.push_back({1,label(e)});return e;}
 uint32_t referenceHandle(uintptr_t e){auto id=label(e);events.calls.push_back({2,id});return active->candidates[id-1].handle;}
 SourcePhysicsSearchTraceCandidate clipToCollideable(const SourceServerHullRayCandidate&ray,uint32_t mask,uintptr_t c){auto id=label(c);require(mask==active->mask&&shortened(ray)==forwarded,"owned clip full ray");events.calls.push_back({3,id});return active->candidates[id-1].trace;}
};
#endif
int main(int argc,char**argv){try{
 require(argc==4,"expected engine or -, input and output");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary),rayOut(std::string(argv[3])+".ray",std::ios::binary);require(bool(in)&&bool(out)&&bool(rayOut),"fixture streams");
#ifdef NATIVE_ENGINE_HULL_TRACE
 AddVectoredExceptionHandler(1,fault);NativePrivateWorldImageFixture image(argv[1]);defaultSurface=*reinterpret_cast<uint32_t*>(image.base+0x402fe8);require(defaultSurface==0x1035067c&&*reinterpret_cast<uint32_t*>(image.base+0x402fec)==0,"default trace surface/metadata");
 uint32_t profile=0;*reinterpret_cast<uint32_t*>(image.base+0x643ee8)|=1;*reinterpret_cast<uint32_t*>(image.base+0x643ee4)=reinterpret_cast<uint32_t>(&profile);*reinterpret_cast<uint32_t*>(image.base+0x31a480)=reinterpret_cast<uint32_t>(&overflow);
 std::array<uintptr_t,28>table{};table[4]=uintptr_t(&clip);table[24]=uintptr_t(&resolve);table[25]=uintptr_t(&worldIndex);table[26]=uintptr_t(&annotateWorld);table[27]=uintptr_t(&partitionMask);std::array<uintptr_t,2>object{uintptr_t(table.data()),0};
 std::array<uintptr_t,15>partitionTable{};partitionTable[14]=uintptr_t(&enumerate);*reinterpret_cast<uintptr_t*>(image.base+0x6683c0)=uintptr_t(partitionTable.data());
 std::array<uintptr_t,3>entityTable{};entityTable[2]=uintptr_t(&getHandle);const std::array<uintptr_t,2>filterTable{uintptr_t(&predicate),uintptr_t(&traceType)};
 image.seal();PrivateX86BoundaryObserverFixture observer({0x10149eb0,0x1016f0d3,0x1016f0d4,0x1016f0d5},boundary);observer.configure(true);
 using Trace=void(__thiscall*)(void*,const void*,uint32_t,const void*,void*);auto trace=reinterpret_cast<Trace>(0x1016ece0);
#else
 Services services;
#endif
 EngineHullInputFixture f;size_t cases=0;
 while(engineHullReadInput(in,f)){
  active=&f;activeCase=cases;events={};forwarded.fill(0xff);SourcePhysicsSearchTraceCandidate result;
#ifdef NATIVE_ENGINE_HULL_TRACE
  entities.clear();for(const auto&c:f.candidates)entities.push_back({uint32_t(uintptr_t(entityTable.data())),c.handle});const auto oldEntities=entities;
  alignas(16)auto ray=f.rawRay;const auto oldRay=ray;std::array<uintptr_t,2>filter{uintptr_t(filterTable.data()),0};const auto oldFilter=filter;std::array<uint8_t,116>output;output.fill(0xa7);
  trace(object.data(),ray.data(),f.mask,filter.data(),output.data()+16);result=readNative(output.data()+16);
  require(ray==oldRay&&entities==oldEntities&&filter==oldFilter,"original mutated ray/entity/filter");for(size_t i=0;i<16;++i)require(output[i]==0xa7&&output[100+i]==0xa7,"trace guard");require(profile==cases+1&&object[1]==cases+1,"profiling count");
#else
  result=sourceEngineHullTraceCandidate(f.ray,f.mask,f.mode,services,[&](uintptr_t entity,uint32_t mask){require(mask==f.mask,"owned predicate mask");auto id=label(entity);events.calls.push_back({4,id});return id!=f.excluded;});
#endif
  enginePointWriteOutput(out,result,events);supportPut(rayOut,forwarded);++cases;
 }
 require(cases>0,"no cases");out.flush();rayOut.flush();require(bool(out)&&bool(rayOut),"output write");
#ifdef NATIVE_ENGINE_HULL_TRACE
 observer.configure(false);image.verify();
#endif
 std::cout<<"PASS engine hull policy cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
