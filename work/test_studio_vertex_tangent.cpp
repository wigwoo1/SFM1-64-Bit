#include "studio_vertex_tangent_candidate.h"
#include <iostream>
#include <limits>
int main(){try{
    static_assert(sizeof(void*)==8,"x64 required");auto require=[](bool b,const char*s){if(!b)throw std::runtime_error(s);};unsigned checks=0,refusals=0;
    auto x=studioVertexTangentCandidate({1,0,0},{0,0,0,-1},false);require(x.repaired&&x.tangent[0]==0&&x.tangent[1]==0&&x.tangent[2]>0.999f&&x.tangent[2]<1&&x.tangent[3]==-1,"repair Y axis/epsilon/sign");++checks;
    auto y=studioVertexTangentCandidate({0,1,0},{0,0,0,1},false);require(y.repaired&&y.tangent[0]==0&&y.tangent[1]==0&&y.tangent[2]<-0.999f,"repair X axis");++checks;
    auto unchanged=studioVertexTangentCandidate({1,0,0},{0,0.1f,0,1},false);require(!unchanged.repaired&&unchanged.tangent==std::array<float,4>{0,0.1f,0,1},"strict repair threshold");++checks;
    auto below=studioVertexTangentCandidate({1,0,0},{0,0.09999999f,0,1},false);require(below.repaired,"below repair threshold");++checks;
    auto raw=studioVertexTangentCandidate({0,0,1},{2,0,0,1},false);require(!raw.repaired&&raw.tangent[0]==2,"do not normalize a valid raw tangent");++checks;
    auto plus=studioVertexTangentCandidate({0,0,1},{1,0,0,1},true),minus=studioVertexTangentCandidate({0,0,1},{1,0,0,-1},true);
    require(plus.packedFrame==std::array<uint8_t,4>{192,192,255,192}&&minus.packedFrame==std::array<uint8_t,4>{192,192,255,63},"packed handedness before truncation");++checks;
    auto reject=[&](auto normal,auto tangent){try{studioVertexTangentCandidate(normal,tangent,true);}catch(const std::invalid_argument&){++refusals;return;}throw std::runtime_error("invalid tangent admitted");};
    const float nan=std::numeric_limits<float>::quiet_NaN(),inf=std::numeric_limits<float>::infinity();
    for(const auto&n:std::array<std::array<float,3>,4>{{{0,0,0},{2,0,0},{nan,0,1},{inf,0,1}}})reject(n,std::array<float,4>{1,0,0,1});
    for(const auto&t:std::array<std::array<float,4>,5>{{{nan,0,0,1},{inf,0,0,1},{10001,0,0,1},{1,0,0,0},{1,0,0,nan}}})reject(std::array<float,3>{0,0,1},t);
    std::cout<<"PASS checks="<<checks<<" refusals="<<refusals<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
