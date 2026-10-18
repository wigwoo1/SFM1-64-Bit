#include "source_bsp_trace_surface_flags_candidate.h"
#include "source_optimized_box_line_ray_candidate.h"
#include "source_bsp_world_light_trace_candidate.h"
#include <fstream>
#include <iterator>
#include <iostream>
int main(int argc,char**argv){try{
    if(argc!=2)throw std::runtime_error("map required");std::ifstream in(argv[1],std::ios::binary);if(!in)throw std::runtime_error("map read");std::vector<uint8_t>b((std::istreambuf_iterator<char>(in)),{});if(b.size()<1036)throw std::runtime_error("map header");uint32_t offset,length;std::memcpy(&offset,b.data()+8+6*16,4);std::memcpy(&length,b.data()+8+6*16+4,4);std::vector<uint32_t>binding(length/72);for(size_t i=0;i<binding.size();++i)binding[i]=uint32_t(binding.size()-1-i);SourceBspTraceSurfaceFlagsCandidate catalog(b,binding);unsigned checks=0;auto require=[](bool x){if(!x)throw std::runtime_error("trace surface assertion");};
    for(size_t i=0;i<binding.size();++i){uint32_t flags;std::memcpy(&flags,b.data()+offset+i*72+64,4);require(catalog.flags({SourceCollisionSurfaceKindCandidate::TextureInfo,uint32_t(i)})==uint16_t(flags)&&catalog.flags({SourceCollisionSurfaceKindCandidate::RuntimeSurface,binding[i]})==uint16_t(flags));}++checks;
    require(catalog.flags({})==0&&catalog.flags({SourceCollisionSurfaceKindCandidate::Displacement,0})==0&&catalog.forTexture(-1)==65535);++checks;
    const std::array<float,3>lo{-1,-1,-1},hi{1,1,1};SourceOptimizedBoxLineStateCandidate state;state.surface={SourceCollisionSurfaceKindCandidate::RuntimeSurface,123};sourceOptimizedBoxLineRayCandidate(lo,hi,{-2,0,0},{0,0,0},1,state);require(state.surface.kind==SourceCollisionSurfaceKindCandidate::BoxFace&&state.surface.index==0);++checks;
    state={};state.fraction=.25f;state.surface={SourceCollisionSurfaceKindCandidate::RuntimeSurface,123};sourceOptimizedBoxLineRayCandidate(lo,hi,{0,0,0},{2,0,0},1,state);require(state.fraction==1&&state.surface.kind==SourceCollisionSurfaceKindCandidate::Default);++checks;
    state={};state.surface={SourceCollisionSurfaceKindCandidate::RuntimeSurface,123};sourceOptimizedBoxLineRayCandidate(lo,hi,{0,0,0},{.5f,0,0},1,state);require(state.allSolid&&state.surface.kind==SourceCollisionSurfaceKindCandidate::RuntimeSurface&&state.surface.index==123);++checks;
    state={};state.surface={SourceCollisionSurfaceKindCandidate::RuntimeSurface,123};sourceOptimizedBoxLineRayCandidate(lo,hi,{-2,3,0},{0,3,0},1,state);require(state.surface.kind==SourceCollisionSurfaceKindCandidate::RuntimeSurface&&state.surface.index==123);++checks;
    auto reject=[&](auto action){bool caught=false;try{action();}catch(const std::runtime_error&){caught=true;}require(caught);++checks;};
    reject([&]{catalog.flags({SourceCollisionSurfaceKindCandidate::BoxFace,0});});
    reject([&]{catalog.flags({SourceCollisionSurfaceKindCandidate::TextureInfo,UINT32_MAX});});
    reject([&]{catalog.flags({SourceCollisionSurfaceKindCandidate::RuntimeSurface,65535});});
    reject([&]{catalog.flags({SourceCollisionSurfaceKindCandidate::RuntimeSurface,65534});});
    auto bad=binding;bad.pop_back();reject([&]{SourceBspTraceSurfaceFlagsCandidate invalid(b,bad);});bad=binding;bad[0]=65536;reject([&]{SourceBspTraceSurfaceFlagsCandidate invalid(b,bad);});
    auto bytes=b;uint32_t wrong=uint32_t(b.size()+1);std::memcpy(bytes.data()+8+6*16,&wrong,4);reject([&]{SourceBspTraceSurfaceFlagsCandidate invalid(bytes,binding);});
    SourceBspBrushOnlyTraceCandidate brushes(b,binding);const std::vector<SourceDisplacementRuntimeMeshCandidate>none;SourcePreparedWorldRayCandidate geometry(brushes,none);SourceBspWorldLightTraceCandidate lighting(geometry,catalog);
    reject([&]{lighting.trace({{0,0,0},{1,0,0},true},0x4481,{SourceWorldLightTraceModeCandidate::EverythingFilterProps,uintptr_t(0x123400001234ull)});});
    reject([&]{lighting.trace({{0,0,0},{1,0,0},false},0x4481,{});});
    std::cout<<"PASS trace surface safety checks="<<checks<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
