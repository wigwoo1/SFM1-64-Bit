#include "ifm_draw_frames_candidate.h"
#include <iostream>
#include <limits>
#include <optional>
struct Services {
    bool model=true,child=true;int32_t count=1;unsigned boundsCalls=0;
    RecoveredMatrix3x4 root{{{1,0,0,10},{0,1,0,20},{0,0,1,30}}},childWorld{{{1,0,0,0},{0,1,0,0},{0,0,1,0}}};
    IfmBoundsCandidate bounds{{{2,4,6},{4,8,10}}};
    std::optional<int>resolveDag(){return model?std::optional<int>(1):std::nullopt;}
    auto world(int n){return n==1?root:childWorld;}int32_t childCount(int){return count;}
    std::optional<int>typedChildZero(int){return child?std::optional<int>(2):std::nullopt;}
    auto innerBounds(const IfmBoundsWrapperFramesCandidate&frames){++boundsCalls;if(frames.position38!=std::array<float,3>{10,20,30})throw std::runtime_error("intermediate frame");return bounds;}
};
int main(){try{
    unsigned checks=0,rejections=0;
    auto require=[&](bool v){if(!v)throw std::runtime_error("draw frame safety check");++checks;};
    auto reject=[&](auto f){try{f();}catch(const std::runtime_error&){++rejections;return;}throw std::runtime_error("expected refusal");};
    Services s;const auto result=ifmDrawFramesCandidate(1,s);require(result.position38==std::array<float,3>{13,26,38}&&result.position50==std::array<float,3>{10,20,30}&&s.boundsCalls==1);
    const float nan=std::numeric_limits<float>::quiet_NaN(),huge=std::numeric_limits<float>::max();
    s=Services{};s.childWorld[0][0]=nan;s.bounds[0][0]=nan;require(ifmDrawFramesCandidate(0,s).position38==std::array<float,3>{10,20,30}&&s.boundsCalls==0);
    s.count=0;require(ifmDrawFramesCandidate(1,s).position38==std::array<float,3>{10,20,30}&&s.boundsCalls==0);
    s.count=1;s.child=false;require(ifmDrawFramesCandidate(1,s).position38==std::array<float,3>{10,20,30}&&s.boundsCalls==0);
    s.model=false;s.root[0][0]=nan;require(ifmDrawFramesCandidate(1,s).position50==std::array<float,3>{0,0,0});
    s=Services{};s.root[0][0]=nan;reject([&]{ifmDrawFramesCandidate(0,s);});
    s=Services{};s.childWorld[0][0]=nan;reject([&]{ifmDrawFramesCandidate(1,s);});
    s=Services{};s.bounds[0][0]=nan;reject([&]{ifmDrawFramesCandidate(1,s);});
    s=Services{};s.bounds[0][0]=s.bounds[1][0]=huge;reject([&]{ifmDrawFramesCandidate(1,s);});
    auto m=Services{}.root;m[0][0]=huge;reject([&]{ifmDecomposeFrameCandidate(m);});
    m={};const auto zero=ifmDecomposeFrameCandidate(m);require(zero.scale==0x1p-23f&&zero.position==std::array<float,3>{0,0,0});
    m=Services{}.root;for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)m[r][c]*=2;const auto scaled=ifmDecomposeFrameCandidate(m);require(scaled.scale==2&&scaled.position==std::array<float,3>{10,20,30});
    std::cout<<"PASS checks="<<checks<<" rejections="<<rejections<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
