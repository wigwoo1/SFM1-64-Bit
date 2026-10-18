#include "source_physics_tetrahedron_simplex_candidate.h"
#include <cstring>
#include <iostream>
#include <limits>

int main(){try{
    using V=std::array<float,3>;unsigned checks=0;auto require=[&](bool pass){if(!pass)throw std::runtime_error("tetrahedron simplex assertion");++checks;};
    struct Case{std::array<V,4>points;uint32_t count;V direction;bool complete;};
    const Case cases[]={
        {{{{-2,1,-1},{0,2,2},{1,-2,0},{1,1,0}}},4,{7,8,9},true},
        {{{{-1,-1,-1},{1,-2,2},{-2,2,1},{1,-2,1}}},3,{-1,-2,0},false},
        {{{{1,-1,0},{1,-2,0},{-2,-1,-2},{-2,0,-1}}},2,{-7,-14,-7},false},
        {{{{0,-2,1},{0,-1,2},{0,2,-1},{2,-1,-2}}},3,{12,6,6},false},
        {{{{1,0,-2},{2,-2,-1},{1,-1,0},{2,-2,-1}}},3,{3,2,1},false},
        {{{{-2,0,-1},{0,-1,1},{0,2,-2},{2,-1,-1}}},3,{-1,-4,-10},false},
        {{{{0,1,-1},{2,-1,2},{2,2,-1},{-1,0,-2}}},3,{9,-9,-9},false},
        {{{{1,0,0},{-1,-2,-2},{0,2,0},{0,0,1}}},2,{1,0,1},false}
    };
    SourcePhysicsSimplexCandidate state;SourcePhysicsSimplexVertexCandidate incoming;V direction;
    auto setup=[&](const Case&test){state={};state.count=UINT32_MAX;for(unsigned j=0;j<3;++j)state.vertices[j]={test.points[j],uint16_t(0x8001+j),uint16_t(0xffff-j)};state.vertices[3]={{5,6,7},0x8abc,0xffff};incoming={test.points[3],0xfedc,0x9abc};direction={7,8,9};};
    for(const auto&test:cases){setup(test);const auto old=state;require(sourcePhysicsTetrahedronSimplexCandidate(state,incoming,direction)==test.complete);require(state.count==test.count&&direction==test.direction);if(test.complete)require(std::memcmp(&state.vertices[3],&incoming,16)==0);else require(std::memcmp(&state.vertices[3],&old.vertices[3],16)==0);}
    setup(cases[0]);direction={std::numeric_limits<float>::quiet_NaN(),-0.0f,std::numeric_limits<float>::infinity()};const auto oldDirection=direction;require(sourcePhysicsTetrahedronSimplexCandidate(state,incoming,direction)&&std::memcmp(&direction,&oldDirection,12)==0);
    setup(cases[1]);state.vertices[3].point[0]=std::numeric_limits<float>::quiet_NaN();const auto oldVertex=state.vertices[3];direction[0]=state.vertices[3].point[0];require(!sourcePhysicsTetrahedronSimplexCandidate(state,incoming,direction)&&std::memcmp(&state.vertices[3],&oldVertex,16)==0&&direction==cases[1].direction);
    setup(cases[0]);for(auto&vertex:state.vertices)vertex.point={0,0,0};incoming.point={0,0,0};require(sourcePhysicsTetrahedronSimplexCandidate(state,incoming,direction)&&state.count==4&&direction==V{7,8,9});
    auto refuse=[&](unsigned which,V point){setup(cases[1]);if(which==3)incoming.point=point;else state.vertices[which].point=point;const auto before=state;const auto beforeDir=direction;bool rejected=false;try{sourcePhysicsTetrahedronSimplexCandidate(state,incoming,direction);}catch(const std::runtime_error&){rejected=true;}require(rejected&&std::memcmp(&before,&state,68)==0&&std::memcmp(&beforeDir,&direction,12)==0);};
    const float nan=std::numeric_limits<float>::quiet_NaN(),inf=std::numeric_limits<float>::infinity(),max=std::numeric_limits<float>::max();
    refuse(0,{nan,0,0});refuse(1,{0,inf,0});refuse(2,{0,0,nan});refuse(3,{inf,0,0});refuse(0,{max,max,max});refuse(2,{1e30f,-1e30f,1e30f});
    std::cout<<"PASS tetrahedron simplex analytical/transaction safety checks="<<checks<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
