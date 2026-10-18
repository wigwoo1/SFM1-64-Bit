#include "ifm_bounds_geometry_services_candidate.h"
#include <iostream>
#include <limits>
struct CompositeSafetyProviders {
    bool cached=false,available=true,clock=false,fill=true;unsigned syncs=0,loads=0,setups=0,clocks=0,worlds=0,scales=0,childQueries=0;
    IfmStudioBoundsCandidate studioData{true,{{{-1,-2,-3},{1,2,3}}},{{{-1,-2,-3},{1,2,3}}},{{{{-1,-2,-3},{1,2,3}}}}};
    std::vector<IfmHitboxCandidate>boxes{{0,{{{-4,-5,-6},{4,5,6}}}}};
    IfmBoundsWrapperFramesCandidate frames{};
    static RecoveredMatrix3x4 identity(){return {{{1,0,0,0},{0,1,0,0},{0,0,1,0}}};}
    void synchronize(){++syncs;}
    void acquire(){if(!cached){++loads;cached=available;}}
    const IfmStudioBoundsCandidate*studio(){acquire();return cached?&studioData:nullptr;}
    IfmBoundsCandidate modelBounds(){return studioData.hull;}
    std::optional<IfmFirstBonePoseCandidate>firstBonePose(){return {};}
    const std::vector<IfmHitboxCandidate>*animatedHitboxes(){acquire();return cached?&boxes:nullptr;}
    bool hasClock(){return clock;}
    int32_t clockTick(){++clocks;return 50000;}
    bool setupBones(std::array<RecoveredMatrix3x4,256>&palette,uint32_t maximum,uint32_t mask,float time){++setups;if(maximum!=256||mask!=0x100||time!=(clock?5.0f:0.0f))throw std::runtime_error("composite setup parameters");if(fill)palette[0]=identity();return false;}
    RecoveredMatrix3x4 modelWorld(){++worlds;return identity();}
    float localScale(){++scales;return 1;}
    std::optional<uint32_t>resolveDag(){++childQueries;return 0;}
    int32_t childCount(uint32_t){return 0;}
    std::optional<uint32_t>typedChildZero(uint32_t){throw std::runtime_error("unexpected child query");}
    bool overrideParentEnabled(uint32_t){throw std::runtime_error("unexpected child override query");}
    RecoveredMatrix3x4 world(uint32_t){throw std::runtime_error("unexpected child world query");}
    RecoveredMatrix3x4 local(uint32_t){throw std::runtime_error("unexpected child local query");}
    const IfmBoundsWrapperFramesCandidate&wrapperFrames(){return frames;}
};
int main(){try{
    auto require=[](bool ok){if(!ok)throw std::runtime_error("composite safety assertion");};unsigned checks=0;
    IfmRenderableBoundsStateCandidate state{true,true,true,true,0,3,0,0,0};
    CompositeSafetyProviders p;IfmBoundsGeometryServicesCandidate<CompositeSafetyProviders>s{p,0};
    require(ifmRenderableBoundsWrapperCandidate(state,s)==p.boxes[0].bounds&&p.loads==1&&p.setups==1&&p.clocks==0);++checks;
    require(ifmRenderableBoundsWrapperCandidate(state,s)==p.boxes[0].bounds&&p.loads==1&&p.setups==2&&p.clocks==0&&p.worlds==2&&p.scales==2);++checks;
    p.clock=true;require(ifmRenderableBoundsCandidate(state,s)==p.boxes[0].bounds&&p.clocks==1&&p.setups==3);++checks;
    p.boxes.clear();const auto oldSetups=p.setups,oldClocks=p.clocks;require(ifmRenderableBoundsCandidate(state,s)==p.studioData.hull&&p.setups==oldSetups&&p.clocks==oldClocks);++checks;
    CompositeSafetyProviders unavailable;unavailable.available=false;IfmBoundsGeometryServicesCandidate<CompositeSafetyProviders>failedLoad{unavailable,0};
    require(ifmRenderableBoundsCandidate(state,failedLoad)==unavailable.studioData.hull&&unavailable.loads==2&&unavailable.setups==0&&unavailable.clocks==0);++checks;
    require(ifmRenderableBoundsCandidate(state,failedLoad)==unavailable.studioData.hull&&unavailable.loads==4&&unavailable.setups==0);++checks;
    CompositeSafetyProviders bad;bad.cached=true;bad.fill=false;IfmBoundsGeometryServicesCandidate<CompositeSafetyProviders>badServices{bad,0};bool rejected=false;try{ifmRenderableBoundsCandidate(state,badServices);}catch(const std::runtime_error&){rejected=true;}require(rejected&&bad.boxes[0].bounds[0][0]==-4&&bad.loads==0);++checks;
    CompositeSafetyProviders bypass;bypass.cached=true;bypass.fill=false;bypass.frames.angles44[0]=std::numeric_limits<float>::quiet_NaN();IfmBoundsGeometryServicesCandidate<CompositeSafetyProviders>skip{bypass,0};auto noModel=state;noModel.modelPresent=false;require(ifmRenderableBoundsWrapperCandidate(noModel,skip)==IfmBoundsCandidate{{{-16,-16,0},{16,16,72}}}&&bypass.syncs==0&&bypass.setups==0);++checks;
    auto noCompute=state;noCompute.computeBounds=false;require(ifmRenderableBoundsWrapperCandidate(noCompute,skip)==bypass.studioData.hull&&bypass.setups==0);++checks;
    CompositeSafetyProviders wrapped;wrapped.frames.position50={10,20,30};IfmBoundsGeometryServicesCandidate<CompositeSafetyProviders>w{wrapped,1};auto wrapperState=noModel;wrapperState.runtimeMode=1;wrapperState.childCount=1;require(ifmRenderableBoundsWrapperCandidate(wrapperState,w)==IfmBoundsCandidate{{{-6,4,30},{26,36,102}}}&&wrapped.syncs==0&&wrapped.setups==0);++checks;
    std::cout<<"PASS complete bounds composition safety checks="<<checks<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
