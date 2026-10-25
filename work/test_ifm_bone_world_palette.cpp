#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "session_ifm_bone_inputs_candidate.h"
#include <iostream>
int main(){try{
    auto require=[](bool good){if(!good)throw std::runtime_error("palette safety assertion");};size_t checks=0,rejections=0;
    auto root=RecoveredMatrix3x4{{{1,0,0,10},{0,1,0,20},{0,0,1,30}}};
    std::vector<IfmBoneLocalPoseCandidate>bones{{-1,0x100,{1,2,3},{0,0,0,1},2},{0,0x100,{4,5,6},{0,0,0,1},1}};
    auto result=ifmBoneWorldPaletteCandidate(bones,root,0x100);require((*result[1])[0][3]==19&&(*result[1])[1][3]==32&&(*result[1])[2][3]==45);++checks;
    const auto before=bones;
    auto reject=[&](auto operation){try{operation();}catch(const std::runtime_error&){++rejections;return;}throw std::runtime_error("invalid palette accepted");};
    for(int mode=0;mode<6;++mode){auto bad=bones;
        if(mode==0)bad.resize(257);if(mode==1)bad[1].parent=1;if(mode==2)bad[0].flags=0;
        if(mode==3)bad[0].orientation[0]=INFINITY;if(mode==4)bad[1].scale=NAN;if(mode==5)bad[1].parent=-2;
        reject([&]{(void)ifmBoneWorldPaletteCandidate(bad,root,0x100);});
    }
    require(std::memcmp(before.data(),bones.data(),bones.size()*sizeof(bones[0]))==0);++checks;
    auto skip=bones;skip[1].flags=0;skip[1].parent=999;skip[1].position[0]=NAN;
    require(!ifmBoneWorldPaletteCandidate(skip,root,0x100)[1]);++checks;
    require(ifmBoneWorldPaletteCandidate({},root,0x100).empty());++checks;
    Graph g;g.strings={"DmeGameModel","DmeTransform","DmElement","bones","position","orientation","scale","model","second-label","first-label"};
    g.elements.resize(3);g.elements[0].type=0;g.elements[1].type=g.elements[2].type=1;g.elements[1].name=8;g.elements[2].name=9;
    Value one,two;one.reference=1;two.reference=2;g.elements[0].attributes.push_back(Attribute{3,15,{one,two}});
    for(size_t i=1;i<3;++i){Value p,q;p.wire.resize(12);q.wire.resize(16);float x=float(i),w=1;std::memcpy(p.wire.data(),&x,4);std::memcpy(q.wire.data()+12,&w,4);g.elements[i].attributes={Attribute{4,10,{p}},Attribute{5,13,{q}}};}
    std::vector<uint8_t>mdl(0x200+2*216);auto put=[&](size_t at,int32_t v){std::memcpy(mdl.data()+at,&v,4);};
    put(0,0x54534449);put(4,49);put(76,int32_t(mdl.size()));put(156,2);put(160,0x200);put(0x204,-1);put(0x200+216+4,0);put(0x200+160,0x100);put(0x200+216+160,0x100);
    auto read=[&]{return sessionIfmBoneInputsCandidate(g,0,mdl,[&](uint32_t id){return g.elements[id].type==1;});};
    auto input=read();require(input[0].position[0]==1&&input[1].position[0]==2);++checks; // labels do not reorder
    g.elements[2].type=2;require(read()[1].position[0]==0&&read()[1].orientation[3]==1);++checks;
    g.elements[0].attributes[0].values.resize(1);require(read()[1].position[0]==0&&read()[1].scale==1);++checks;
    g.elements[0].attributes[0].values[0].reference=UINT32_MAX;require(read()[0].position[0]==0);++checks;
    g.elements[0].attributes[0].values[0].reference=UINT32_MAX-1;reject([&]{(void)read();});
    g.elements[0].attributes[0].values[0].reference=1;g.elements[1].attributes[0].type=3;reject([&]{(void)read();});
    put(156,257);reject([&]{(void)read();});
    std::cout<<"PASS checks="<<checks<<" rejections="<<rejections<<" positional_bones=1 no_name_remap=1\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
