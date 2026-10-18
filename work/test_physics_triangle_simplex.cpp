#include "source_physics_triangle_simplex_candidate.h"
#include <cstring>
#include <iostream>
#include <limits>

int main(){try{
    unsigned checks=0;auto require=[&](bool pass){if(!pass)throw std::runtime_error("triangle simplex assertion");++checks;};
    SourcePhysicsSimplexCandidate state;std::array<float,3>direction{};
    SourcePhysicsSimplexVertexCandidate p;
    auto reset=[&](){state={};state.count=UINT32_MAX;state.vertices[0]={{1,0,0},0x8001,0xffff};state.vertices[1]={{0,1,0},0xabcd,0x8000};state.vertices[2]={{7,8,9},0x1234,0x4321};state.vertices[3]={{-1,-2,-3},0xffff,0x8000};p={{-1,-1,1},0xfedc,0x9abc};direction={7,8,9};};
    auto same=[](const auto&a,const auto&b){return std::memcmp(&a,&b,sizeof(a))==0;};
    reset();auto before=state;require(!sourcePhysicsTriangleSimplexCandidate(state,p,direction));
    require(state.count==3&&direction==std::array<float,3>{1,1,3});
    require(same(state.vertices[0],before.vertices[0])&&same(state.vertices[1],p)&&same(state.vertices[2],before.vertices[1])&&same(state.vertices[3],before.vertices[3]));
    reset();p.point[2]=-1;before=state;require(!sourcePhysicsTriangleSimplexCandidate(state,p,direction));
    require(state.count==3&&direction==std::array<float,3>{1,1,-3});
    require(same(state.vertices[0],before.vertices[0])&&same(state.vertices[1],before.vertices[1])&&same(state.vertices[2],p)&&same(state.vertices[3],before.vertices[3]));
    reset();p.point={-1,1,0};before=state;require(!sourcePhysicsTriangleSimplexCandidate(state,p,direction));
    require(state.count==2&&direction==std::array<float,3>{1,2,0}&&same(state.vertices[1],p));
    require(same(state.vertices[0],before.vertices[0])&&same(state.vertices[2],before.vertices[2])&&same(state.vertices[3],before.vertices[3]));
    reset();p.point={1,-1,0};before=state;require(!sourcePhysicsTriangleSimplexCandidate(state,p,direction));
    require(state.count==2&&direction==std::array<float,3>{2,1,0}&&same(state.vertices[0],p));
    require(same(state.vertices[1],before.vertices[1])&&same(state.vertices[2],before.vertices[2])&&same(state.vertices[3],before.vertices[3]));
    reset();state.vertices[0].point={2,0,0};state.vertices[1].point={3,0,0};p.point={1,0,0};before=state;
    require(!sourcePhysicsTriangleSimplexCandidate(state,p,direction)&&state.count==1&&direction==p.point&&same(state.vertices[0],p));
    require(same(state.vertices[1],before.vertices[1])&&same(state.vertices[2],before.vertices[2])&&same(state.vertices[3],before.vertices[3]));
    reset();state.vertices[0].point={1,0,0};state.vertices[1].point={-1,0,0};p.point={0,0,0};
    require(!sourcePhysicsTriangleSimplexCandidate(state,p,direction)&&state.count==1&&direction==p.point);
    reset();state.vertices[2].point[0]=std::numeric_limits<float>::quiet_NaN();state.vertices[3].point[0]=std::numeric_limits<float>::infinity();before=state;direction[0]=state.vertices[2].point[0];
    require(!sourcePhysicsTriangleSimplexCandidate(state,p,direction)&&same(state.vertices[3],before.vertices[3])&&direction==std::array<float,3>{1,1,3});
    auto refuse=[&](std::array<float,3>a,std::array<float,3>b,std::array<float,3>incoming){reset();state.vertices[0].point=a;state.vertices[1].point=b;p.point=incoming;const auto saved=state;const auto oldDirection=direction;bool rejected=false;try{sourcePhysicsTriangleSimplexCandidate(state,p,direction);}catch(const std::runtime_error&){rejected=true;}require(rejected&&same(state,saved)&&same(direction,oldDirection));};
    const float inf=std::numeric_limits<float>::infinity(),nan=std::numeric_limits<float>::quiet_NaN(),max=std::numeric_limits<float>::max();
    refuse({inf,0,0},{0,1,0},{-1,1,0});refuse({1,0,0},{0,nan,0},{-1,1,0});refuse({1,0,0},{0,1,0},{-1,inf,0});
    refuse({max,0,0},{0,1,0},{-max,0,0});refuse({1e20f,0,0},{0,1e20f,0},{-1,-1,1});refuse({1e10f,0,0},{0,1e10f,0},{-1e10f,-1e10f,1e10f});
    std::cout<<"PASS triangle simplex analytical/transaction safety checks="<<checks<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
