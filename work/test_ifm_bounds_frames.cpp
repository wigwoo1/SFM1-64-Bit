#include "ifm_bounds_frames_candidate.h"
#include <iostream>
#include <limits>
struct FrameGuardServices {
    unsigned resolves=0,counts=0,children=0,predicates=0,worlds=0,locals=0;
    bool resolved=true,typed=true,overridden=false;int32_t count=1;
    RecoveredMatrix3x4 matrix{{{1,0,0,0},{0,1,0,0},{0,0,1,0}}};
    std::optional<uint32_t>resolveDag(){++resolves;return resolved?std::optional<uint32_t>(0):std::nullopt;}
    int32_t childCount(uint32_t){++counts;return count;}
    std::optional<uint32_t>typedChildZero(uint32_t){++children;return typed?std::optional<uint32_t>(1):std::nullopt;}
    bool overrideParentEnabled(uint32_t){++predicates;return overridden;}
    RecoveredMatrix3x4 world(uint32_t){++worlds;return matrix;}
    RecoveredMatrix3x4 local(uint32_t){++locals;return matrix;}
};
int main(){try{
    auto require=[](bool ok){if(!ok)throw std::runtime_error("frame safety assertion");};unsigned checks=0;
    auto rejects=[&](auto fn){bool caught=false;try{fn();}catch(const std::runtime_error&){caught=true;}require(caught);++checks;};
    const float infinity=std::numeric_limits<float>::infinity(),nan=std::numeric_limits<float>::quiet_NaN();
    const IfmBoundsCandidate box{{{-1,-2,-3},{1,2,3}}};
    for(int32_t mode:{0,1,-1}){FrameGuardServices s;if(mode)s.resolved=false;const auto b=ifmBoundsAdjustChildCandidate(mode,s,box);require(b==box&&s.resolves==1&&s.counts==0&&s.children==0);++checks;}
    for(int32_t count:{0,-1,INT32_MIN}){FrameGuardServices s;s.count=count;require(ifmBoundsAdjustChildCandidate(1,s,box)==box&&s.counts==1&&s.children==0);++checks;}
    FrameGuardServices missing;missing.typed=false;rejects([&]{ifmBoundsAdjustChildCandidate(1,missing,box);});require(missing.children==1&&missing.predicates==0&&missing.locals==0&&missing.worlds==0);++checks;
    FrameGuardServices local;require(ifmBoundsAdjustChildCandidate(1,local,box)==box&&local.locals==1&&local.worlds==0);++checks;
    FrameGuardServices over;over.overridden=true;require(ifmBoundsAdjustChildCandidate(1,over,box)==box&&over.locals==0&&over.worlds==2);++checks;
    for(bool disabled:{false,true})for(bool resolved:{false,true}){require(ifmBoundsChildOverrideCandidate(disabled,resolved)==(!disabled&&resolved));++checks;}
    for(float invalid:{infinity,nan,36001.0f,-36001.0f})rejects([&]{ifmBoundsAngleMatrixCandidate({invalid,0,0},{0,0,0});});
    rejects([&]{ifmBoundsAngleMatrixCandidate({0,0,0},{infinity,0,0});});
    FrameGuardServices bad;bad.matrix[0][0]=nan;rejects([&]{ifmBoundsAdjustChildCandidate(1,bad,box);});
    FrameGuardServices singular;singular.overridden=true;singular.matrix={};rejects([&]{ifmBoundsAdjustChildCandidate(1,singular,box);});
    FrameGuardServices overflow;overflow.matrix[0][1]=std::numeric_limits<float>::max();rejects([&]{ifmBoundsAdjustChildCandidate(1,overflow,box);});
    const auto before=box;FrameGuardServices badBounds;auto invalidBox=box;invalidBox[0][0]=infinity;const auto invalidBefore=invalidBox;
    rejects([&]{ifmBoundsAdjustChildCandidate(1,badBounds,invalidBox);});require(invalidBox==invalidBefore&&box==before);++checks;
    // Unused input bypass: mode0 copies even invalid bounds without evaluating
    // geometry; the selected-transform path above rejects the same input.
    FrameGuardServices bypass;const auto unchanged=ifmBoundsAdjustChildCandidate(0,bypass,invalidBox);require(unchanged==invalidBefore&&bypass.locals==0&&bypass.worlds==0);++checks;
    std::cout<<"PASS bounds frame safety checks="<<checks<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
