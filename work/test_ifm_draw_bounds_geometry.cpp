#include "ifm_draw_bounds_geometry_candidate.h"
#include "ifm_virtual_studio_bounds_candidate.h"
#include <iostream>
struct DrawProviders {
    IfmStudioBoundsCandidate input;bool child=true,allowFallback=false,poseAvailable=false;unsigned calls=0;IfmBoundsWrapperFramesCandidate accepted{};std::vector<IfmHitboxCandidate>boxes;
    auto resolveDag(){return std::optional<uint32_t>(0);}int32_t childCount(uint32_t){return 1;}auto typedChildZero(uint32_t){return child?std::optional<uint32_t>(1):std::nullopt;}
    auto world(uint32_t id){return RecoveredMatrix3x4{{{1,0,0,id?15.0f:10.0f},{0,1,0,20},{0,0,1,30}}};}
    bool overrideParentEnabled(uint32_t){return false;}auto local(uint32_t){return RecoveredMatrix3x4{{{1,0,0,5},{0,1,0,0},{0,0,1,0}}};}
    void synchronize(){++calls;}auto studio(){return &input;}IfmBoundsCandidate modelBounds(){if(!allowFallback)throw std::runtime_error("unexpected engine fallback");return input.hull;}
    std::optional<IfmFirstBonePoseCandidate>firstBonePose(){return {};}
    const std::vector<IfmHitboxCandidate>*animatedHitboxes(){if(!poseAvailable)throw std::runtime_error("unsupported test pose provider");return &boxes;}
    bool hasClock(){return true;}int32_t clockTick(){return 0;}
    bool setupBones(std::array<RecoveredMatrix3x4,256>&,uint32_t,uint32_t,float){throw std::runtime_error("unsupported test bone setup");}
    auto modelWorld(){return world(0);}float localScale(){return 1;}auto wrapperFrames(){return accepted;}
    void acceptIntermediateFrames(const IfmBoundsWrapperFramesCandidate&f){accepted=f;}
};
int main(){try{
    unsigned checks=0,refusals=0;auto check=[&](bool v){if(!v)throw std::runtime_error("draw bounds check");++checks;};auto reject=[&](auto f){try{f();}catch(const std::runtime_error&){++refusals;return;}throw std::runtime_error("expected draw bounds refusal");};
    std::vector<uint8_t>root(0x400);auto word=[&](size_t at,uint32_t v){std::memcpy(root.data()+at,&v,4);};word(0,0x54534449);word(4,49);word(76,uint32_t(root.size()));word(0xbc,1);word(0xc0,0x200);word(0x204,212);word(0x20c,0x800);std::memcpy(root.data()+0x2d4,"root",5);
    IfmBoundsCandidate hull{{{-1,-2,-3},{1,2,3}}};std::memcpy(root.data()+128,hull.data(),24);
    const auto m=readSourceMdlSequenceMetadataCandidate(root);auto included=m;included.sequences[0].flags=0;included.sequences[0].bounds={-10,-10,-10,10,10,10};
    const auto studio=ifmVirtualStudioBoundsCandidate(root,{m,included},SourceSequenceNameCacheModeCandidate::Enabled);
    check(studio.hull==hull&&studio.sequences.size()==1&&studio.sequences[0]==IfmBoundsCandidate{{{-10,-10,-10},{10,10,10}}});
    DrawProviders p{studio};IfmRenderableBoundsStateCandidate s;s.modelPresent=s.rawModelResolved=s.gameModelTyped=true;s.category=3;s.sequence=0;s.runtimeMode=1;
    const auto result=ifmDrawBoundsGeometryCandidate(s,p);check(result.boundsQueried&&result.frames.position38==std::array<float,3>{15,20,30}&&result.frames.position50==std::array<float,3>{10,20,30});
    check(result.framesAtBounds.position38==std::array<float,3>{10,20,30}&&p.accepted.position38==result.framesAtBounds.position38&&p.calls==1);
    p.calls=0;p.input.hull[0][0]=std::numeric_limits<float>::quiet_NaN();s.runtimeMode=0;check(!ifmDrawBoundsGeometryCandidate(s,p).boundsQueried&&p.calls==0);
    s.runtimeMode=1;p.child=false;check(!ifmDrawBoundsGeometryCandidate(s,p).boundsQueried&&p.calls==0);
    p.child=true;reject([&]{ifmDrawBoundsGeometryCandidate(s,p);});
    p.input=studio;s.renderableFlags=0x20;reject([&]{ifmDrawBoundsGeometryCandidate(s,p);});
    auto wrong=m;wrong.sequences[0].label="other";reject([&]{ifmVirtualStudioBoundsCandidate(root,{wrong},SourceSequenceNameCacheModeCandidate::Disabled);});
    included.sequences[0].bounds[0]=std::numeric_limits<float>::infinity();reject([&]{ifmVirtualStudioBoundsCandidate(root,{m,included},SourceSequenceNameCacheModeCandidate::Enabled);});
    check(readSourceMdlSequenceMetadataCandidate(root).sequences[0].label=="root");
    std::cout<<"PASS checks="<<checks<<" refusals="<<refusals<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
