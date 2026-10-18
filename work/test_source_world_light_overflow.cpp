#include "source_world_light_overflow_candidate.h"
#include <iostream>
#include <limits>
struct Services {
    std::array<std::array<float,3>,6>basis{{{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}}};
    int32_t count=6;unsigned visibilityCalls=0,basisCalls=0,countCalls=0;uint32_t flags=99;float attenuation=1;
    SourceWorldLightVisibilityCandidate visibility(const SourceWorldLightAngularCandidate&,const std::array<float,3>&,uint32_t f){++visibilityCalls;flags=f;return {{0,0,1},attenuation};}
    const auto&directions(){++basisCalls;return basis;}
    int32_t directionCount(){++countCalls;return count;}
};
int main(){try{
    unsigned checks=0;auto require=[](bool b){if(!b)throw std::runtime_error("worldlight overflow safety assertion");};SourceWorldLightAngularCandidate l;l.normal={0,0,-.5f};l.intensity={2,4,6};l.inner=.75f;l.outer=.25f;
    for(int type=0;type<6;++type){l.type=type;const float value=sourceWorldLightAngularCandidate(l,l.normal,{0,0,1},{0,0,1});require(value==(type==0||type==3||type==2?.5f:1.f));++checks;}
    l.type=2;l.exponent=2;require(sourceWorldLightAngularCandidate(l,l.normal,{0,0,1},{0,0,1})==.25f);++checks;
    l.exponent=0;require(sourceWorldLightAngularCandidate(l,l.normal,{0,0,1},{0,0,1})==.5f);++checks;
    l.normal={0,0,-.25f};require(sourceWorldLightAngularCandidate(l,l.normal,{0,0,1},{0,0,1})==0);++checks;
    l.type=1;SourceModelLightingStateCandidate state;state.lights={uintptr_t(0x123400000001ull)};Services s;sourceWorldLightOverflowCandidate(l,{1,2,3},state,true,s);require(s.visibilityCalls==1&&s.flags==3&&s.basisCalls==1&&s.countCalls==1&&state.ambient[12]==2&&state.ambient[13]==4&&state.ambient[14]==6&&state.lights[0]==0x123400000001ull);++checks;
    Services zero;zero.attenuation=0;const auto before=state.ambient;sourceWorldLightOverflowCandidate(l,{0,0,0},state,false,zero);require(state.ambient==before&&zero.visibilityCalls==1&&zero.basisCalls==0&&zero.countCalls==0&&zero.flags==0);++checks;
    Services none;none.count=0;sourceWorldLightOverflowCandidate(l,{0,0,0},state,false,none);require(state.ambient==before&&none.basisCalls==1&&none.countCalls==1);++checks;
    Services bad;bad.count=7;bool rejected=false;try{sourceWorldLightOverflowCandidate(l,{0,0,0},state,false,bad);}catch(const std::runtime_error&){rejected=true;}require(rejected);++checks;
    Services invalid;invalid.attenuation=std::numeric_limits<float>::infinity();rejected=false;try{sourceWorldLightOverflowCandidate(l,{0,0,0},state,false,invalid);}catch(const std::runtime_error&){rejected=true;}require(rejected);++checks;
    std::cout<<"PASS world light overflow safety checks="<<checks<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
