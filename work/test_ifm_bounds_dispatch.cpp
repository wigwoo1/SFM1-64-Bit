#include "ifm_renderable_bounds_candidate.h"
#include <iostream>
struct Services {
    IfmBoundsCandidate model{{{-1,-2,-3},{1,2,3}}};std::optional<IfmBoundsCandidate>animated;
    IfmStudioBoundsCandidate data;bool hasStudio=true;std::optional<RecoveredMatrix3x4>child;std::optional<IfmFirstBonePoseCandidate>bone;RecoveredMatrix3x4 wrapper{};
    unsigned syncCalls=0,animatedCalls=0,modelCalls=0,childCalls=0,boneCalls=0,wrapperCalls=0;
    Services(){data.hull=model;data.view=model;data.sequences={model};for(unsigned i=0;i<3;++i)wrapper[i][i]=1;}
    void synchronize(){++syncCalls;}std::optional<IfmBoundsCandidate>animatedBounds(){++animatedCalls;return animated;}
    const IfmStudioBoundsCandidate*studio(){return hasStudio?&data:nullptr;}
    IfmBoundsCandidate modelBounds(){++modelCalls;return model;}
    std::optional<RecoveredMatrix3x4>childTransform(){++childCalls;return child;}
    std::optional<IfmFirstBonePoseCandidate>firstBonePose(){++boneCalls;return bone;}
    RecoveredMatrix3x4 wrapperTransform(){++wrapperCalls;return wrapper;}
};
int main(){try{
    IfmRenderableBoundsStateCandidate state;Services initial;
    if(ifmRenderableBoundsCandidate(state,initial)!=IfmBoundsCandidate{{{-16,-16,0},{16,16,72}}}||initial.syncCalls||initial.childCalls||initial.modelCalls)throw std::runtime_error("default box/bypass");
    state.modelPresent=true;state.category=1;Services brush;auto translated=brush.wrapper;translated[0][3]=10;brush.child=translated;
    if(ifmRenderableBoundsCandidate(state,brush)!=IfmBoundsCandidate{{{9,-2,-3},{11,2,3}}}||brush.modelCalls!=1||brush.childCalls!=1||brush.syncCalls||brush.boneCalls)throw std::runtime_error("category1 child adjustment");
    state.category=3;state.rawModelResolved=state.gameModelTyped=state.computeBounds=true;state.sequence=99;Services hitbox;hitbox.animated=hitbox.model;
    if(ifmRenderableBoundsCandidate(state,hitbox)!=hitbox.model||hitbox.animatedCalls!=1||hitbox.childCalls||hitbox.boneCalls||hitbox.modelCalls)throw std::runtime_error("animated success bypass");
    Services stored;stored.data.hull={};stored.data.view=stored.model;stored.bone=IfmFirstBonePoseCandidate{{10,0,0},{0,0,0,1}};
    if(ifmRenderableBoundsCandidate(state,stored)!=IfmBoundsCandidate{{{-1,-2,-3},{11,2,3}}}||stored.boneCalls!=1||stored.childCalls!=1)throw std::runtime_error("view fallback/sequence0/first bone union");
    state.sequence=-1;Services missing;missing.bone=stored.bone;
    if(ifmRenderableBoundsCandidate(state,missing)!=missing.model||missing.modelCalls!=1||missing.boneCalls)throw std::runtime_error("invalid sequence fallback bypass bone");
    state.modelPresent=false;state.gameModelTyped=false;state.runtimeMode=1;state.childCount=1;Services wrapper;wrapper.wrapper[0][3]=100;
    if(ifmRenderableBoundsWrapperCandidate(state,wrapper)!=IfmBoundsCandidate{{{84,-16,0},{116,16,72}}}||wrapper.wrapperCalls!=1||wrapper.syncCalls)throw std::runtime_error("wrapper raw-node default transform");
    IfmBoundsCandidate zeros{};for(auto&v:zeros)v.fill(-0.0f);const auto retained=ifmExpandBoundsCandidate(zeros,IfmBoundsCandidate{});if(std::memcmp(zeros.data(),retained.data(),24))throw std::runtime_error("native union signed-zero tie");
    unsigned guards=0;auto reject=[&](auto action){bool rejected=false;try{action();}catch(const std::exception&){rejected=true;}if(!rejected)throw std::runtime_error("invalid selected bounds accepted");++guards;};
    state.modelPresent=true;state.category=1;state.runtimeMode=0;
    reject([&]{Services s;s.model[0][0]=INFINITY;ifmRenderableBoundsCandidate(state,s);});
    reject([&]{Services s;auto m=s.wrapper;m[0][0]=INFINITY;s.child=m;ifmRenderableBoundsCandidate(state,s);});
    state.category=3;state.gameModelTyped=true;state.sequence=0;
    reject([&]{Services s;s.data.sequences.clear();ifmRenderableBoundsCandidate(state,s);});
    reject([&]{Services s;s.data.sequences[0][0][0]=INFINITY;ifmRenderableBoundsCandidate(state,s);});
    reject([&]{Services s;s.bone=IfmFirstBonePoseCandidate{};s.bone->orientation[0]=INFINITY;ifmRenderableBoundsCandidate(state,s);});
    state.modelPresent=false;state.runtimeMode=1;
    reject([&]{Services s;s.wrapper[0][0]=INFINITY;ifmRenderableBoundsWrapperCandidate(state,s);});
    std::vector<uint8_t>mdl(408);auto integer=[&](size_t at,int32_t v){std::memcpy(mdl.data()+at,&v,4);};integer(0,0x54534449);integer(4,49);integer(76,408);integer(188,1);integer(192,196);std::memcpy(mdl.data()+104,initial.model.data(),24);std::memcpy(mdl.data()+128,initial.model.data(),24);std::memcpy(mdl.data()+228,initial.model.data(),24);
    const auto parsed=readIfmMdlStudioBoundsCandidate(mdl);if(parsed.hull!=initial.model||parsed.view!=initial.model||parsed.sequences.size()!=1||parsed.sequences[0]!=initial.model)throw std::runtime_error("MDL bounds fixture");
    reject([&]{auto b=mdl;b.resize(100);readIfmMdlStudioBoundsCandidate(b);});
    reject([&]{auto b=mdl;b[4]=48;readIfmMdlStudioBoundsCandidate(b);});
    reject([&]{auto b=mdl;b[76]=0;readIfmMdlStudioBoundsCandidate(b);});
    reject([&]{auto b=mdl;const int32_t offset=10000;std::memcpy(b.data()+192,&offset,4);readIfmMdlStudioBoundsCandidate(b);});
    reject([&]{auto b=mdl;const int32_t count=-1;std::memcpy(b.data()+188,&count,4);readIfmMdlStudioBoundsCandidate(b);});
    reject([&]{auto b=mdl;const float bad=INFINITY;std::memcpy(b.data()+104,&bad,4);readIfmMdlStudioBoundsCandidate(b);});
    reject([&]{auto b=mdl;const float bad=INFINITY;std::memcpy(b.data()+228,&bad,4);readIfmMdlStudioBoundsCandidate(b);});
    if(guards!=13)throw std::runtime_error("guard count");std::cout<<"PASS analytical dispatch/wrapper, lazy bypass, hull-zero/view, sequence clamp, first-bone union gate, raw-node wrapper gate, signed-zero union ties, MDL49 reader and thirteen guards\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
