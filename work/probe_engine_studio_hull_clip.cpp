#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>
#include "physics_search_test_fixture.h"
#include "source_studio_physics_contents_candidate.h"
#ifdef NATIVE_ENGINE_STUDIO_HULL_CLIP
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
#include "native_vphysics_library_fixture.h"
#include "native_physics_search_fixture.h"
#include "private_x86_boundary_observer_fixture.h"
#else
#include "source_engine_studio_hull_clip_candidate.h"
#endif
struct Query {std::array<uint8_t,80> rawRay{};std::array<float,3> origin{},angles{};uint32_t mask=0,flags=0,cache=0,handle=0,surface=0;uint64_t world=0;};
static std::ofstream* contentsEvents=nullptr;static uint32_t currentQuery=0;
static void recordContents(uint32_t gameData,uint32_t contents){supportPut(*contentsEvents,currentQuery);supportPut(*contentsEvents,gameData);supportPut(*contentsEvents,contents);}
static std::string queryBytes(const Query&q){std::ostringstream out(std::ios::binary);supportPut(out,q.rawRay);for(const auto&v:{q.origin,q.angles})supportPut(out,v);for(uint32_t x:{q.mask,q.flags,q.cache,q.handle,q.surface})supportPut(out,x);supportPut(out,q.world);return out.str();}
static bool readQuery(std::istream& in,Query& q){
    if(in.peek()==std::char_traits<char>::eof())return false;
    supportGet(in,q.rawRay);supportGet(in,q.origin);supportGet(in,q.angles);supportGet(in,q.mask);supportGet(in,q.flags);supportGet(in,q.cache);supportGet(in,q.handle);supportGet(in,q.surface);supportGet(in,q.world);
    if(q.rawRay[68]>1||q.rawRay[69]>1||q.cache>2||q.surface>65535||(q.handle&0xffff0000u)!=0x40000000u||(q.mask&0x40000000u))throw std::runtime_error("studio hull query controls");
    const uint32_t custom=q.rawRay[68]?1u:2u,allowed=4u|128u|256u|(q.rawRay[68]?2u:1u);
    if((q.flags&custom)||(q.flags&~allowed))throw std::runtime_error("studio hull flags unsupported");
    for(unsigned at:{0u,16u,32u,48u})for(unsigned axis=0;axis<3;++axis){float value;std::memcpy(&value,q.rawRay.data()+at+axis*4,4);if(!std::isfinite(value)||(at==48&&value<0))throw std::runtime_error("studio hull ray domain");}
    for(const auto& values:{q.origin,q.angles})for(float value:values)if(!std::isfinite(value))throw std::runtime_error("studio hull pose domain");
    for(float angle:q.angles)if(angle< -36000||angle>36000)throw std::runtime_error("studio hull angle domain");return true;
}
#ifdef NATIVE_ENGINE_STUDIO_HULL_CLIP
static Query*active=nullptr;static uint8_t*modelData=nullptr;static std::vector<uint8_t>*studioData=nullptr;static void*collideCache=nullptr;static void*emptyCache=nullptr;static void*handleEntity=nullptr;static NativeVphysicsLibraryFixture*physics=nullptr;static uint8_t*engineBase=nullptr;static uint32_t physicsCalls=0;static size_t activeCase=0;
static const uint8_t* nativeContentsArgument=nullptr;static bool contentsPending=false;static uint32_t pendingGameData=0;static const void* nativeExpectedShape=nullptr;
static const uint8_t* __fastcall getModel(void*,void*){return modelData;}
static const uint8_t* __fastcall getStudio(void*,void*,const void*m){if(m!=modelData)throw std::runtime_error("studio model binding");return studioData->data();}
static void* __fastcall getCache(void*,void*,uint16_t id){if(id!=7)throw std::runtime_error("studio cache key");return active->cache==1?nullptr:active->cache==2?emptyCache:collideCache;}
static const float* __fastcall origin(void*,void*){return active->origin.data();}
static const float* __fastcall angles(void*,void*){return active->angles.data();}
static uint32_t __fastcall solidType(void*,void*){return 6;}
static uint32_t __fastcall solidFlags(void*,void*){return active->flags;}
static const void* __fastcall rootTransform(void*,void*){static const float identity[12]{1,0,0,0,0,1,0,0,0,0,1,0};return identity;}
static void* __fastcall getHandleEntity(void*,void*){return handleEntity;}
static const uint32_t* __fastcall referenceHandle(void*,void*){return &active->handle;}
static void* __fastcall getWorldEntity(void*,void*,int index){if(index)throw std::runtime_error("world entity index");return reinterpret_cast<void*>(uintptr_t(active->world?0x60000001u:0));}
static void __fastcall tracePhysics(void*,void*,const uint8_t*ray,uint32_t mask,const uint8_t*contents,const void*shape,const float*pos,const float*rot,void*out){
    if(mask!=active->mask||shape!=nativeExpectedShape||!contents||supportReadNative<uint32_t>(contents,0)!=reinterpret_cast<uint32_t>(engineBase+0x354470)||supportReadNative<const uint8_t*>(contents,4)!=studioData->data()||std::memcmp(pos,active->origin.data(),12)||std::memcmp(rot,active->angles.data(),12))throw std::runtime_error("actual engine physics binding");auto expected=active->rawRay;if(active->flags&256u){const auto root=uint32_t(uintptr_t(rootTransform(nullptr,nullptr)));std::memcpy(expected.data()+64,&root,4);}if(std::memcmp(ray,expected.data(),80))throw std::runtime_error("actual engine full hull ray binding");nativeContentsArgument=contents;++physicsCalls;using Trace=void(__thiscall*)(void*,const void*,uint32_t,const void*,const void*,const float*,const float*,void*);reinterpret_cast<Trace>(physics->table[31])(physics->object,ray,mask,contents,shape,pos,rot,out);
}
static void observeContents(unsigned index,CONTEXT& context){
    if(index==0&&context.Eip==uintptr_t(engineBase)+0x16d6c0){
        const auto* stack=reinterpret_cast<const uint32_t*>(context.Esp);
        if(contentsPending||context.Ecx!=uintptr_t(nativeContentsArgument)||stack[0]!=uintptr_t(physics->module)+0x2a360||supportReadNative<const uint8_t*>(nativeContentsArgument,4)!=studioData->data())throw std::runtime_error("original studio contents entry");
        pendingGameData=stack[1];contentsPending=true;
    }else if(index==1&&context.Eip==uintptr_t(physics->module)+0x2a360){
        if(!contentsPending)throw std::runtime_error("original studio contents return without entry");recordContents(pendingGameData,context.Eax);contentsPending=false;
    }else throw std::runtime_error("unexpected original studio contents observation");
    context.EFlags|=0x10000u;
}
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"studio clip fault case="<<activeCase<<" eip="<<std::hex<<p->ContextRecord->Eip<<std::endl;ExitProcess(90);}
#endif
static std::vector<uint8_t>read(const char*path){std::ifstream in(path,std::ios::binary);if(!in)throw std::runtime_error("studio/PHY file read");return {(std::istreambuf_iterator<char>(in)),{}};}
int main(int argc,char**argv){try{
    if(argc!=7)throw std::runtime_error("engine DLL-directory PHY MDL queries output required");const auto phyBytes=read(argv[3]),mdlBytes=read(argv[4]);const SourceStudioPhysicsContentsCandidate studio(mdlBytes);std::ifstream queries(argv[5],std::ios::binary);std::ofstream out(argv[6],std::ios::binary),meta(std::string(argv[6])+".events",std::ios::binary),contents(std::string(argv[6])+".contents",std::ios::binary);if(!queries||!out||!meta||!contents)throw std::runtime_error("studio hull clip streams");contentsEvents=&contents;
#ifdef NATIVE_ENGINE_STUDIO_HULL_CLIP
    static_assert(sizeof(void*)==4,"original x86");AddVectoredExceptionHandler(1,fault);NativePrivateWorldImageFixture engine(argv[1]);engineBase=engine.base;NativeVphysicsLibraryFixture library(argv[2]);physics=&library;if(library.rva(library.table[31])!=0x5e00||library.rva(library.table[36])!=0x90a0||library.rva(library.table[37])!=0x5eb0)throw std::runtime_error("pinned engine physics vtable");const auto physicsEmpty=searchDefaultSurfaceNative(library);const uint32_t engineEmpty=*reinterpret_cast<uint32_t*>(engine.base+0x402fe8);if(engineEmpty!=0x1035067c||*reinterpret_cast<uint32_t*>(engine.base+0x402fec)!=0)throw std::runtime_error("pinned engine default surface");std::vector<uint8_t>runtimeStudio=mdlBytes;studioData=&runtimeStudio;std::array<uint8_t,0x138>model{};modelData=model.data();supportWriteNative(model.data(),0x110,3u);supportWriteNative(model.data(),0x134,uint16_t(7));std::array<uintptr_t,6>modelInfoTable{};modelInfoTable[5]=reinterpret_cast<uintptr_t>(&getStudio);uintptr_t modelInfo=reinterpret_cast<uintptr_t>(modelInfoTable.data());*reinterpret_cast<uintptr_t*>(engine.base+0x401484)=reinterpret_cast<uintptr_t>(&modelInfo);
    std::array<uintptr_t,16>cacheTable{};cacheTable[15]=reinterpret_cast<uintptr_t>(&getCache);uintptr_t cacheObject=reinterpret_cast<uintptr_t>(cacheTable.data());*reinterpret_cast<uintptr_t*>(engine.base+0x694200)=reinterpret_cast<uintptr_t>(&cacheObject);std::array<uintptr_t,32>physicsTable{};physicsTable[31]=reinterpret_cast<uintptr_t>(&tracePhysics);uintptr_t physicsObject=reinterpret_cast<uintptr_t>(physicsTable.data());*reinterpret_cast<uintptr_t*>(engine.base+0x6377fc)=reinterpret_cast<uintptr_t>(&physicsObject);
    std::array<uintptr_t,18>collideableTable{};collideableTable[0]=reinterpret_cast<uintptr_t>(&getHandleEntity);collideableTable[7]=reinterpret_cast<uintptr_t>(&getModel);collideableTable[8]=reinterpret_cast<uintptr_t>(&origin);collideableTable[9]=reinterpret_cast<uintptr_t>(&angles);collideableTable[11]=reinterpret_cast<uintptr_t>(&solidType);collideableTable[12]=reinterpret_cast<uintptr_t>(&solidFlags);collideableTable[17]=reinterpret_cast<uintptr_t>(&rootTransform);uintptr_t collideable=reinterpret_cast<uintptr_t>(collideableTable.data());std::array<uintptr_t,3>entityTable{};entityTable[2]=reinterpret_cast<uintptr_t>(&referenceHandle);uintptr_t entity=reinterpret_cast<uintptr_t>(entityTable.data());handleEntity=&entity;std::array<uintptr_t,4>entitiesTable{};entitiesTable[3]=reinterpret_cast<uintptr_t>(&getWorldEntity);uintptr_t entitiesObject=reinterpret_cast<uintptr_t>(entitiesTable.data());*reinterpret_cast<uintptr_t*>(engine.base+0x4b55a0)=reinterpret_cast<uintptr_t>(&entitiesObject);
    std::array<uintptr_t,27>engineTable{};engineTable[26]=reinterpret_cast<uintptr_t>(engine.base+0x16d7b0);uintptr_t engineObject=reinterpret_cast<uintptr_t>(engineTable.data());alignas(16)std::array<uint8_t,32>vc;vc.fill(0xa7);const uint32_t header=supportReadNative<uint32_t>(phyBytes.data(),0);if(header<16||header>=phyBytes.size()||supportReadNative<uint32_t>(phyBytes.data(),8)!=1||phyBytes.size()-header>INT32_MAX)throw std::runtime_error("native one-solid PHY span");using Load=void(__thiscall*)(void*,void*,int,const void*,int,bool);using Unload=void(__thiscall*)(void*,void*);reinterpret_cast<Load>(library.table[36])(library.object,vc.data()+8,1,phyBytes.data()+header,int(phyBytes.size()-header),false);if((supportReadNative<uint16_t>(vc.data(),8)&0x7fff)!=1)throw std::runtime_error("native PHY not loaded");const auto actualShapes=supportReadNative<void**>(vc.data(),12);if(!actualShapes||!actualShapes[0])throw std::runtime_error("actual studio physics shape");nativeExpectedShape=actualShapes[0];collideCache=vc.data()+8;std::array<uint8_t,16>empty{};emptyCache=empty.data();engine.seal();using Clip=void(__thiscall*)(void*,const void*,uint32_t,void*,void*);const auto clip=reinterpret_cast<Clip>(engine.base+0x16f300);PrivateX86BoundaryObserverFixture observer({uint32_t(uintptr_t(engine.base)+0x16d6c0),uint32_t(uintptr_t(library.module)+0x2a360),uint32_t(uintptr_t(engine.base)+0x16d6ee),uint32_t(uintptr_t(engine.base)+0x16d6ef)},observeContents);
    const auto managerBefore=std::array<uint32_t,3>{*reinterpret_cast<uint32_t*>(engine.base+0x40b598),*reinterpret_cast<uint32_t*>(engine.base+0x40b59c),*reinterpret_cast<uint32_t*>(engine.base+0x40b5a0)};
#else
    static_assert(sizeof(void*)==8,"owned AMD64");const SourcePhyHullTraceCandidate shape(phyBytes);
#endif
    Query q;size_t cases=0;while(readQuery(queries,q)){if(cases>UINT32_MAX)throw std::runtime_error("studio hull query index");currentQuery=uint32_t(cases);const auto before=queryBytes(q);SourcePhysicsSearchTraceCandidate result;uint32_t dispatched=0;
#ifdef NATIVE_ENGINE_STUDIO_HULL_CLIP
        active=&q;activeCase=cases;physicsCalls=0;supportWriteNative(runtimeStudio.data(),396,uint16_t(q.surface));alignas(16)auto ray=q.rawRay;const auto oldRay=ray;alignas(16)std::array<uint8_t,128>trace;trace.fill(0xa7);contentsPending=false;nativeContentsArgument=nullptr;observer.configure(true);clip(&engineObject,ray.data(),q.mask,&collideable,trace.data()+16);observer.configure(false);if(contentsPending)throw std::runtime_error("unfinished original contents callback");result=searchReadNativeTrace(trace.data()+16,engineEmpty);const uint32_t surface=supportReadNative<uint32_t>(trace.data()+16,60);if(surface==physicsEmpty)result.surfaceId=0;else if(surface==reinterpret_cast<uint32_t>(engine.base+0x3545f8))result.surfaceId=UINT64_MAX;if(result.entityId){if(result.entityId!=0x60000001u)throw std::runtime_error("native studio world identity");result.entityId=q.world;}dispatched=physicsCalls;if(dispatched>1||ray!=oldRay)throw std::runtime_error("native dispatch count/ray restoration");for(size_t i=0;i<16;++i)if(trace[i]!=0xa7)throw std::runtime_error("leading studio trace canary");for(size_t i=100;i<trace.size();++i)if(trace[i]!=0xa7)throw std::runtime_error("trailing studio trace canary");
#else
        SourceEngineStudioHullClipParametersCandidate p;p.origin=q.origin;p.angles=q.angles;p.mask=q.mask;p.solidFlags=q.flags;p.referenceHandle=q.handle;p.surfaceProperty=uint16_t(q.surface);p.worldEntityId=q.world;p.cache=static_cast<SourceEngineStudioHullCacheCandidate>(q.cache);p.identityRootEstablished=true;
        SourceServerHullRayCandidate ray;std::memcpy(ray.center.data(),q.rawRay.data(),12);std::memcpy(ray.delta.data(),q.rawRay.data()+16,12);std::memcpy(ray.offset.data(),q.rawRay.data()+32,12);std::memcpy(ray.extents.data(),q.rawRay.data()+48,12);ray.isRay=q.rawRay[68]!=0;ray.swept=q.rawRay[69]!=0;
        const auto answer=sourceEngineStudioHullClipCandidate(q.cache?nullptr:&shape,studio,ray,p,[](uint32_t gameData,uint32_t value){recordContents(gameData,value);});result=answer.trace;dispatched=answer.physicsDispatched;
#endif
        searchWriteTrace(out,result);supportPut(meta,dispatched);const uint32_t hit=result.fraction<1||result.allSolid||result.startSolid;supportPut(meta,hit);if(queryBytes(q)!=before)throw std::runtime_error("studio query changed");++cases;
    }if(!cases)throw std::runtime_error("empty studio queries");out.flush();meta.flush();contents.flush();if(!out||!meta||!contents)throw std::runtime_error("studio hull outputs");
#ifdef NATIVE_ENGINE_STUDIO_HULL_CLIP
    reinterpret_cast<Unload>(library.table[37])(library.object,vc.data()+8);for(size_t i=0;i<8;++i)if(vc[i]!=0xa7||vc[24+i]!=0xa7)throw std::runtime_error("native vcollide canary");supportWriteNative(runtimeStudio.data(),396,supportReadNative<uint16_t>(mdlBytes.data(),396));if(runtimeStudio!=mdlBytes)throw std::runtime_error("runtime studio data changed");if(std::memcmp(engine.base+0x40b598,managerBefore.data(),12))throw std::runtime_error("original static manager changed");engine.verify();library.verify();VirtualFree(engine.base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS actual engine studio hull clip cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
