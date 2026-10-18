#include "source_physics_refinement_candidate.h"
#include <cstring>
#include <iostream>
#include <limits>
int main(){try{
    unsigned checks=0;auto require=[&](bool ok){if(!ok)throw std::runtime_error("physics refinement assertion");++checks;};SourcePhysicsBoxSupportCandidate box;for(unsigned j=0;j<3;++j)box.axes[j]={float(j+1),float(-int(j)-1)};
    box.point=false;for(unsigned mask=0;mask<8;++mask){std::array<float,3>d;for(unsigned j=0;j<3;++j)d[j]=(mask&(1u<<j))?-0.f:0.f;const auto r=sourcePhysicsBoxSupportCandidate(box,d);require(r.feature==mask);for(unsigned j=0;j<3;++j)require(r.point[j]==box.axes[j][(mask>>j)&1]);}
    box.point=true;const auto point=sourcePhysicsBoxSupportCandidate(box,{std::numeric_limits<float>::quiet_NaN(),0,0});require(point.feature==0&&point.point==std::array<float,3>{});
    SourcePhysicsSimplexCandidate s;SourcePhysicsSupportContextCandidate shape;SourcePhysicsSupportCacheCandidate cache;cache.entries.fill(0xa7);cache.generation=65535;std::array<float,16>ray;ray.fill(std::numeric_limits<float>::quiet_NaN());
    for(uint32_t count:{0u,1u,2u,3u,0xffffffffu}){s.count=count;for(auto&v:s.vertices)v.point.fill(std::numeric_limits<float>::quiet_NaN());const auto before=s;const auto oldCache=cache;require(sourcePhysicsRefinementCandidate(s,ray,shape,box,cache)==0);require(std::memcmp(&s,&before,sizeof(s))==0);require(cache.entries==oldCache.entries&&cache.generation==oldCache.generation);}
    s.count=4;s.vertices[0].point={1,0,0};s.vertices[1].point={0,1,0};s.vertices[2].point={0,0,1};s.vertices[3].point={-1,-1,-1};ray.fill(0);ray[9]=1;shape.mode=2;shape.points={{{1,0,0}},{{0,1,0}},{{0,0,1}},{{-1,-1,-1}}};const auto initial=s;
    for(unsigned variant=0;variant<4;++variant){s=initial;auto badRay=ray;auto badShape=shape;if(variant==0)badRay[0]=std::numeric_limits<float>::quiet_NaN();if(variant==1)badShape.points.clear();if(variant==2)badShape.points[0][0]=std::numeric_limits<float>::quiet_NaN();if(variant==3){badShape.mode=0;badShape.inverseBasis={1,0,0,0,1,0,0,0,1};badShape.transform={1,0,0,0,0,1,0,0,0,0,1,0};badShape.triangles={{{0,0x00060000,0x000d0001,0x00060002}},{{0,0x00060000,0x00070003,0x7ffa0001}},{{0,0x7ffa0000,0x00040002,0x7ffa0003}},{{0,0x7ff90001,0x7ffc0003,0x7ff30002}}};badShape.points[0][0]=std::numeric_limits<float>::quiet_NaN();}const auto old=s;const auto oldCache=cache;bool failed=false;try{sourcePhysicsRefinementCandidate(s,badRay,badShape,box,cache);}catch(const std::runtime_error&){failed=true;}require(failed);require(std::memcmp(&s,&old,sizeof(s))==0);require(cache.entries==oldCache.entries&&cache.generation==oldCache.generation);}
    std::cout<<"PASS physics refinement analytical/transaction checks="<<checks<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
