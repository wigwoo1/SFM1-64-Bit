#include "source_physics_support_candidate.h"
#include <cstring>
#include <iostream>
#include <limits>
int main(){try{
    unsigned checks=0;auto require=[&](bool pass){if(!pass)throw std::runtime_error("physics support assertion");++checks;};
    SourcePhysicsSupportContextCandidate c;c.mode=2;c.points={{0,0,0},{1,0,0},{0,0,0},{0,0,0},{1,1,0},{0,0,0},{0,0,0},{0,0,0}};
    SourcePhysicsSupportCacheCandidate cache;cache.entries.fill(0xa7a7);cache.generation=0xffff;const auto before=cache;auto r=sourcePhysicsSupportCandidate(c,{1,0,0},cache);
    require(r.feature==4&&r.point==c.points[4]);require(cache.entries==before.entries&&cache.generation==before.generation);
    r=sourcePhysicsSupportCandidate(c,{0,0,0},cache);require(r.feature==0&&r.point==c.points[0]);
    c.inverseBasis[0]=std::numeric_limits<float>::quiet_NaN();require(sourcePhysicsSupportCandidate(c,{1,0,0},cache).feature==4);
    c.mode=1;c.points={{0,0,0},{1,0,0},{1,2,0},{0,1,0}};c.firstVertex=17;c.inverseBasis={1,0,0,0,1,0,0,0,1};c.transform={1,0,0,3,0,1,0,4,0,0,1,5};
    r=sourcePhysicsSupportCandidate(c,{1,0,0},cache);require(r.feature==18&&r.point==std::array<float,3>{4,4,5});require(cache.entries==before.entries&&cache.generation==before.generation);
    r=sourcePhysicsSupportCandidate(c,{0,1,0},cache);require(r.feature==17); // SourceY -> IVPZ, all equal.
    auto refused=[&](SourcePhysicsSupportContextCandidate candidate,std::array<float,3>d){const auto old=cache;bool rejected=false;try{sourcePhysicsSupportCandidate(candidate,d,cache);}catch(const std::runtime_error&){rejected=true;}require(rejected&&cache.entries==old.entries&&cache.generation==old.generation);};
    auto bad=c;bad.points.clear();refused(bad,{1,0,0});bad=c;bad.mode=3;refused(bad,{1,0,0});bad=c;bad.firstVertex=32767;refused(bad,{1,0,0});bad=c;bad.points[0][0]=std::numeric_limits<float>::infinity();refused(bad,{1,0,0});bad=c;bad.transform[0]=std::numeric_limits<float>::quiet_NaN();refused(bad,{1,0,0});refused(c,{std::numeric_limits<float>::infinity(),0,0});bad=c;bad.points[1][0]=1e30f;refused(bad,{1e30f,0,0});bad=c;bad.mode=2;bad.points.pop_back();refused(bad,{1,0,0});bad=c;bad.mode=0;bad.triangles={{{0,0x7ffe0000,0,0}}};refused(bad,{1,0,0});
    // A closed tetrahedron: late output-transform overflow must roll back a
    // visit-cache update, including the generation-wrap clearing operation.
    c.mode=0;c.firstVertex=0;c.seeded=false;c.points={{1,1,1},{-1,-1,1},{-1,1,-1},{1,-1,-1}};
    c.triangles={{{0,0x00060000,0x000d0001,0x00060002}},{{0,0x00060000,0x00070003,0x7ffa0001}},{{0,0x7ffa0000,0x00040002,0x7ffa0003}},{{0,0x7ff90001,0x7ffc0003,0x7ff30002}}};
    c.transform={1,0,0,0,0,1,0,0,0,0,1,0};cache.generation=0xffff;
    r=sourcePhysicsSupportCandidate(c,{1,0,0},cache);require(r.feature==0&&r.point==c.points[0]&&cache.generation==0);require(cache.entries[0]==0&&cache.entries[1]==0&&cache.entries[1023]==0);
    cache.generation=0xffff;cache.entries.fill(0xa7a7);bad=c;bad.transform[0]=std::numeric_limits<float>::max();bad.transform[3]=std::numeric_limits<float>::max();refused(bad,{1,0,0});
    std::cout<<"PASS physics support analytical/transaction safety checks="<<checks<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
