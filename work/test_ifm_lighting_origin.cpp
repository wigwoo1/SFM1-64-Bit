#include "ifm_model_lighting_origin_candidate.h"
#include <iostream>
#include <limits>
int main(){try{
    const std::array<float,3>dag{7,-3,2};const std::array<std::array<float,3>,2>bounds{{{-4,-8,10},{8,2,22}}};
    auto unusedIndex=[]()->int32_t{throw std::runtime_error("unused header queried");};
    auto unusedDag=[](uint32_t)->std::array<float,3>{throw std::runtime_error("unused DAG queried");};
    auto unusedBounds=[]()->std::array<std::array<float,3>,2>{throw std::runtime_error("unused bounds queried");};
    auto p=ifmModelLightingOriginCandidate(true,4,true,0x20,unusedIndex,[&](uint32_t id){if(id!=4)throw std::runtime_error("DAG id");return dag;},unusedBounds);if(!p||*p!=dag)throw std::runtime_error("explicit DAG precedence");
    if(ifmModelLightingOriginCandidate(true,{},true,0x20,unusedIndex,unusedDag,unusedBounds))throw std::runtime_error("present-null used absent fallback");
    if(ifmModelLightingOriginCandidate(false,4,true,0x20,[]{return 1;},unusedDag,unusedBounds))throw std::runtime_error("positive attachment used bounds");
    if(ifmModelLightingOriginCandidate(false,4,false,0xdf,[]{return 0;},unusedDag,unusedBounds))throw std::runtime_error("unrelated flag used bounds");
    for(auto mode:{0,1}){p=ifmModelLightingOriginCandidate(false,4,mode==0,mode==1?0x20:0,[]{return -1;},unusedDag,[&]{return bounds;});if(!p||*p!=std::array<float,3>{2,-3,16})throw std::runtime_error("bounds midpoint");}
    auto reversed=bounds;std::swap(reversed[0],reversed[1]);p=ifmModelLightingOriginCandidate(false,{},true,0,[]{return 0;},unusedDag,[&]{return reversed;});if(!p||*p!=std::array<float,3>{2,-3,16})throw std::runtime_error("reversed bounds native arithmetic");
    unsigned guards=0;auto reject=[&](auto action){bool rejected=false;try{action();}catch(const std::exception&){rejected=true;}if(!rejected)throw std::runtime_error("invalid selected input accepted");++guards;};
    const float inf=std::numeric_limits<float>::infinity(),max=std::numeric_limits<float>::max();
    reject([&]{ifmModelLightingOriginCandidate(true,0,false,0,unusedIndex,[&](uint32_t){return std::array<float,3>{inf,0,0};},unusedBounds);});
    reject([&]{auto bb=bounds;bb[0][0]=inf;ifmModelLightingOriginCandidate(false,{},true,0,[]{return 0;},unusedDag,[&]{return bb;});});
    reject([&]{auto bb=bounds;bb[1][1]=inf;ifmModelLightingOriginCandidate(false,{},true,0,[]{return 0;},unusedDag,[&]{return bb;});});
    reject([&]{const std::array<std::array<float,3>,2>bb{{{max,0,0},{max,0,0}}};ifmModelLightingOriginCandidate(false,{},true,0,[]{return 0;},unusedDag,[&]{return bb;});});
    if(guards!=4)throw std::runtime_error("guard count");
    std::cout<<"PASS IFM origin precedence, absent/present-null distinction, positive attachment and unused-callback bypass, computeBounds/flag20, reversed bounds, four finite/overflow guards\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
