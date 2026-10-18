#include "ifm_animated_hitbox_bounds_candidate.h"
#include <iostream>
int main(){try{
    RecoveredMatrix3x4 identity{};for(unsigned i=0;i<3;++i)identity[i][i]=1;
    const IfmBoundsCandidate input{{{-1,-2,-3},{1,2,3}}};auto matrix=identity;matrix[0][0]=-2;matrix[1][1]=3;matrix[2][2]=4;matrix[0][3]=10;matrix[1][3]=20;matrix[2][3]=30;
    if(ifmTransformBoundsCandidate(matrix,input)!=IfmBoundsCandidate{{{8,14,18},{12,26,42}}})throw std::runtime_error("analytical affine bounds");
    std::vector<IfmHitboxCandidate>boxes{{0,input}},empty;
    auto unusedSetup=[](auto&,auto,auto,float)->bool{throw std::runtime_error("unused setup");};auto unusedWorld=[]()->RecoveredMatrix3x4{throw std::runtime_error("unused world");};auto unusedScale=[]()->float{throw std::runtime_error("unused scale");};
    if(ifmAnimatedHitboxBoundsCandidate(nullptr,true,50000,unusedSetup,unusedWorld,unusedScale)||ifmAnimatedHitboxBoundsCandidate(&empty,true,50000,unusedSetup,unusedWorld,unusedScale))throw std::runtime_error("empty hitbox gate");
    auto setup=[&](auto&palette,uint32_t maximum,uint32_t mask,float time){if(maximum!=256||mask!=0x100||time!=5)throw std::runtime_error("setup arguments");palette[0]=identity;return false;};
    const auto out=ifmAnimatedHitboxBoundsCandidate(&boxes,true,50000,setup,[&]{return identity;},[]{return 1.0f;});if(!out||*out!=input)throw std::runtime_error("ignored setup boolean/native output");
    unsigned guards=0;auto reject=[&](auto action){bool rejected=false;try{action();}catch(const std::exception&){rejected=true;}if(!rejected)throw std::runtime_error("invalid bounds accepted");++guards;};
    reject([&]{auto bad=boxes;bad[0].bone=256;ifmAnimatedHitboxBoundsCandidate(&bad,true,50000,unusedSetup,unusedWorld,unusedScale);});
    reject([&]{auto bad=boxes;bad[0].bounds[0][0]=INFINITY;ifmAnimatedHitboxBoundsCandidate(&bad,true,50000,unusedSetup,unusedWorld,unusedScale);});
    reject([&]{ifmAnimatedHitboxBoundsCandidate(&boxes,true,50000,[](auto&,auto,auto,float){return false;},[&]{return identity;},[]{return 1.0f;});});
    reject([&]{ifmAnimatedHitboxBoundsCandidate(&boxes,true,50000,setup,[&]{return identity;},[]{return 0.0f;});});
    reject([&]{auto bad=identity;bad[0][0]=INFINITY;ifmAnimatedHitboxBoundsCandidate(&boxes,true,50000,setup,[&]{return bad;},[]{return 1.0f;});});
    reject([&]{RecoveredMatrix3x4 bad{};ifmAnimatedHitboxBoundsCandidate(&boxes,true,50000,setup,[&]{return bad;},[]{return 1.0f;});});
    reject([&]{auto bb=input;bb[0][0]=bb[1][0]=std::numeric_limits<float>::max();ifmTransformBoundsCandidate(identity,bb);});
    std::vector<uint8_t>mdl(260);auto integer=[&](size_t at,int32_t v){std::memcpy(mdl.data()+at,&v,4);};integer(0,0x54534449);integer(4,49);integer(76,260);integer(156,1);integer(172,1);integer(176,180);integer(184,1);integer(188,12);std::memcpy(mdl.data()+200,input.data(),24);
    auto parsed=readIfmMdlHitboxesCandidate(mdl);if(parsed.boneCount!=1||parsed.firstSet.size()!=1||parsed.firstSet[0].bounds!=input)throw std::runtime_error("MDL fixture parse");
    reject([&]{auto bad=mdl;bad.resize(100);readIfmMdlHitboxesCandidate(bad);});
    reject([&]{auto bad=mdl;bad[4]=48;readIfmMdlHitboxesCandidate(bad);});
    reject([&]{auto bad=mdl;bad[76]=0;readIfmMdlHitboxesCandidate(bad);});
    reject([&]{auto bad=mdl;const int32_t bone=1;std::memcpy(bad.data()+192,&bone,4);readIfmMdlHitboxesCandidate(bad);});
    reject([&]{auto bad=mdl;const int32_t offset=10000;std::memcpy(bad.data()+188,&offset,4);readIfmMdlHitboxesCandidate(bad);});
    reject([&]{auto bad=mdl;const float low=100;std::memcpy(bad.data()+200,&low,4);readIfmMdlHitboxesCandidate(bad);});
    if(guards!=13)throw std::runtime_error("guard count");std::cout<<"PASS analytical affine/animated bounds, empty bypass, max256 mask100 time, ignored filled-setup boolean, MDL49 fixture and thirteen malformed/nonfinite/overflow guards\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
