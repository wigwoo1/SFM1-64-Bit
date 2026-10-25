#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "session_ifm_bone_references_candidate.h"
#include <iostream>
int main(){try{
    unsigned checks=0,refusals=0;auto check=[&](bool v){if(!v)throw std::runtime_error("precompute check");++checks;};auto reject=[&](auto f){try{f();}catch(const std::runtime_error&){++refusals;return;}throw std::runtime_error("precompute refusal expected");};
    RecoveredMatrix3x4 identity{{{1,0,0,0},{0,1,0,0},{0,0,1,0}}};IfmBoneDagReferenceCandidate active;active.overridePresent=true;active.world=identity;active.world[0][3]=10;
    auto disabled=active;disabled.overridesDisabled=true;auto wrongFile=active;wrongFile.sameFile=false;
    std::vector<IfmBoneTransformReferencesCandidate>refs(2);refs[0].references={disabled,active};refs[1].references={wrongFile,active};
    const auto pre=ifmBonePrecomputeCandidate(true,true,refs);check(pre.selectedReferences==std::vector<uint32_t>{0,1}&&!pre.palette[0]&&pre.palette[1]&&(*pre.palette[1])[0][3]==10);
    auto no=ifmBonePrecomputeCandidate(false,true,refs);check(no.selectedReferences==std::vector<uint32_t>{UINT32_MAX,UINT32_MAX}&&!no.palette[1]);
    std::vector<IfmBoneLocalPoseCandidate>bones(2);bones[0].flags=0;bones[1].flags=0x100;bones[1].parent=0;bones[1].position={1,2,3};
    std::vector<std::optional<RecoveredMatrix3x4>>prepared(2);prepared[0]=active.world;
    const auto result=ifmBoneWorldPaletteCandidate(bones,identity,0x100,prepared);check(result[0]&&result[1]&&(*result[1])[0][3]==11&&(*result[1])[1][3]==2);
    bones[0].position[0]=std::numeric_limits<float>::quiet_NaN();check(ifmBoneWorldPaletteCandidate(bones,identity,0,prepared)[0].has_value());
    reject([&]{ifmBoneWorldPaletteCandidate(bones,identity,0,std::vector<std::optional<RecoveredMatrix3x4>>(1));});
    auto invalid=prepared;(*invalid[0])[0][0]=std::numeric_limits<float>::infinity();reject([&]{ifmBoneWorldPaletteCandidate(bones,identity,0,invalid);});
    reject([&]{ifmBoneWorldPaletteCandidate(bones,identity,0x100);});
    reject([&]{ifmBonePrecomputeCandidate(true,true,std::vector<IfmBoneTransformReferencesCandidate>(257));});
    Graph g;g.strings={"DmeGameModel","DmeTransform","DmeDag","bones","transform","overrideParent","disableOverride"};g.elements.resize(4);g.elements[0].type=0;g.elements[1].type=1;g.elements[2].type=g.elements[3].type=2;
    Value bone;bone.reference=1;g.elements[0].attributes.push_back(Attribute{3,15,{bone}});g.elements[2].attributes.push_back(Attribute{4,1,{bone}});
    auto dag=[&](uint32_t id){return g.elements.at(id).type==2;};auto transform=[&](uint32_t id){return g.elements.at(id).type==1;};unsigned worldCalls=0;auto world=[&](uint32_t){++worldCalls;return identity;};
    auto snapshot=[&]{return sessionIfmBoneReferencesCandidate(g,0,1,dag,transform,world);};check(!ifmBonePrecomputeCandidate(true,true,snapshot()).palette[0]&&worldCalls==0);
    Value parent;parent.reference=3;g.elements[2].attributes.push_back(Attribute{5,1,{parent}});check(ifmBonePrecomputeCandidate(true,true,snapshot()).palette[0].has_value()&&worldCalls==1);
    g.elements[3].attributes.push_back(Attribute{4,1,{bone}});reject([&]{snapshot();});g.elements[3].attributes.clear();
    g.elements[2].attributes.back().values[0].reference=0xfffffffe;reject([&]{snapshot();});g.elements[2].attributes.pop_back();
    reject([&]{sessionIfmBoneReferencesCandidate(g,0,2,dag,transform,world);});
    std::cout<<"PASS checks="<<checks<<" refusals="<<refusals<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
