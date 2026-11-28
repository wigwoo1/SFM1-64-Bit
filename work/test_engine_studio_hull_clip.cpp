#include <fstream>
#include <iostream>
#include <iterator>
#include "source_engine_studio_hull_clip_candidate.h"
static void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
static std::vector<uint8_t> read(const char* path){std::ifstream input(path,std::ios::binary);require(bool(input),"safety input read");return {(std::istreambuf_iterator<char>(input)),{}};}
int main(int argc,char** argv){try{
    static_assert(sizeof(void*)==8,"owned AMD64 safety");require(argc==3,"PHY and MDL required");
    const auto phy=read(argv[1]),mdl=read(argv[2]);const auto oldPhy=phy,oldMdl=mdl;const SourcePhyHullTraceCandidate shape(phy);const SourceStudioPhysicsContentsCandidate studio(mdl);
    SourceServerHullRayCandidate ray;ray.center={1,2,3};ray.delta={4,5,6};ray.extents={2,4,8};ray.offset={-1,-2,-3};ray.swept=true;
    SourceEngineStudioHullClipParametersCandidate parameters;parameters.mask=1;parameters.referenceHandle=0x40000001u;
    unsigned checks=0,callbackCount=0;
    auto reject=[&](const SourcePhyHullTraceCandidate* binding,const SourceServerHullRayCandidate& value,const SourceEngineStudioHullClipParametersCandidate& p){bool rejected=false;try{(void)sourceEngineStudioHullClipCandidate(binding,studio,value,p,[&](uint32_t,uint32_t){++callbackCount;});}catch(const std::runtime_error&){rejected=true;}require(rejected&&callbackCount==0,"invalid studio hull contract was accepted/dispatched");++checks;};
    reject(nullptr,ray,parameters);
    auto p=parameters;p.cache=SourceEngineStudioHullCacheCandidate::KnownMissing;reject(&shape,ray,p);
    p.cache=SourceEngineStudioHullCacheCandidate::KnownEmpty;reject(&shape,ray,p);
    p.cache=static_cast<SourceEngineStudioHullCacheCandidate>(3);reject(nullptr,ray,p);
    p=parameters;p.solidFlags=256;reject(&shape,ray,p);
    p=parameters;p.solidFlags=2;reject(&shape,ray,p);
    auto point=ray;point.isRay=true;p.solidFlags=1;reject(&shape,point,p);
    p=parameters;p.mask=0x40000000u;reject(&shape,ray,p);
    p=parameters;p.solidFlags=8;reject(&shape,ray,p);
    p=parameters;p.referenceHandle=0;reject(&shape,ray,p);
    for(const auto state:{SourceEngineStudioHullCacheCandidate::KnownMissing,SourceEngineStudioHullCacheCandidate::KnownEmpty}){
        p=parameters;p.cache=state;const auto result=sourceEngineStudioHullClipCandidate(nullptr,studio,ray,p,[&](uint32_t,uint32_t){++callbackCount;});
        require(!result.physicsDispatched&&result.trace.fraction==1&&!result.trace.allSolid&&!result.trace.startSolid&&!callbackCount,"known absent cache behavior");++checks;
    }
    p=parameters;p.mask=0;p.solidFlags=256;p.identityRootEstablished=true;
    const auto result=sourceEngineStudioHullClipCandidate(&shape,studio,ray,p,[&](uint32_t,uint32_t){++callbackCount;});require(!result.physicsDispatched&&result.trace.fraction==1&&!callbackCount,"studio contents rejection dispatched");++checks;
    require(phy==oldPhy&&mdl==oldMdl,"safety asset bytes changed");std::cout<<"PASS studio hull binding safety checks="<<checks<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
