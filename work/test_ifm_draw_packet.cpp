#include "ifm_model_draw_packet_candidate.h"
#include <iostream>
#include <limits>
int main(){try{
    static_assert(sizeof(uintptr_t)==8,"owned packet test is AMD64");unsigned checks=0;auto require=[](bool ok){if(!ok)throw std::runtime_error("draw packet safety assertion");};
    auto build=[&](float scale,std::optional<std::array<float,3>>origin=std::nullopt){return ifmModelDrawPacketCandidate({10,20,30},{0,0,0},scale,uintptr_t(0x123400001111ull),uintptr_t(0x567800002222ull),0x80000001,-7,9,65535,origin);};
    const auto p=build(2);require(p.modelToWorld==RecoveredMatrix3x4{{{2,0,0,10},{0,2,0,20},{0,0,2,30}}}&&p.renderable==0x123400001111ull&&p.model==0x567800002222ull&&p.entityIndex==-1&&p.hitboxSet==0&&p.skin==-7&&p.body==9&&p.instance==65535&&p.flags==0x80000001);++checks;
    const auto zero=build(0);require(zero.modelToWorld[0][3]==10&&zero.modelToWorld[1][1]==0);++checks;
    const auto neg=build(-2);require(neg.modelToWorld[0][0]==-2&&neg.modelToWorld[0][3]==10);++checks;
    auto original=build(1,std::array<float,3>{7,8,9});const auto copy=original;original.modelToWorld[0][0]=5;original.lightingOrigin->at(0)=999;require(copy.modelToWorld[0][0]==1&&copy.lightingOrigin->at(0)==7);++checks;
    unsigned unusedQueries=0;auto unused=[&](int32_t,auto&)->bool{++unusedQueries;return false;};
    require(ifmModelDrawLightingPointCandidate(p,{1,2,3},0,unused)==std::array<float,3>{12,24,36});++checks;
    require(ifmModelDrawLightingPointCandidate(copy,{1,2,3},0,unused)==std::array<float,3>{7,8,9}&&unusedQueries==0);++checks;
    unsigned queries=0;const auto attached=ifmModelDrawLightingPointCandidate(p,{1,2,3},1,[&](int32_t i,auto&m){++queries;require(i==1);m={{{1,0,0,100},{0,1,0,200},{0,0,1,300}}};return true;});require(attached==std::array<float,3>{101,202,303}&&queries==1);++checks;
    auto rejects=[&](auto fn){bool caught=false;try{fn();}catch(const std::runtime_error&){caught=true;}require(caught);++checks;};const float bad=std::numeric_limits<float>::infinity();
    rejects([&]{build(bad);});rejects([&]{build(1,std::array<float,3>{bad,0,0});});
    rejects([&]{ifmModelDrawPacketCandidate({bad,0,0},{0,0,0},1,1,2,0,0,0,0,{});});
    rejects([&]{ifmModelDrawPacketCandidate({0,0,0},{36001,0,0},1,1,2,0,0,0,0,{});});
    std::cout<<"PASS draw packet safety checks="<<checks<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
