#include "studio_skin_palette_candidate.h"
#include <iostream>
int main(){try{
    unsigned checks=0,refusals=0;auto check=[&](bool v){if(!v)throw std::runtime_error("studio skin check");++checks;};auto reject=[&](auto f){try{f();}catch(const std::runtime_error&){++refusals;return;}throw std::runtime_error("studio skin refusal expected");};
    RecoveredMatrix3x4 id{{{1,0,0,0},{0,1,0,0},{0,0,1,0}}},root{{{0,-1,0,10},{1,0,0,20},{0,0,1,30}}};auto bind=id;bind[0][3]=-5;StudioSkinPaletteMetadataCandidate m;m.flags={0x100,0};m.inverseBind={bind,id};
    auto r=studioSkinPaletteCandidate(m,{root,id},0x100,{id,id});check(r[0][0][3]==10&&r[0][1][3]==15&&r[1]==id);
    m.inverseBind[1][0][0]=std::numeric_limits<float>::quiet_NaN();check(studioSkinPaletteCandidate(m,{root,id},0,{id,id})==std::vector<RecoveredMatrix3x4>{id,id});
    m.staticModel=true;check(studioSkinPaletteCandidate(m,{root},0,{id,id})==std::vector<RecoveredMatrix3x4>{root,id});
    std::vector<uint8_t>mdl(0x800);auto word=[&](size_t at,uint32_t v){std::memcpy(mdl.data()+at,&v,4);};word(0,0x54534449);word(4,49);word(76,uint32_t(mdl.size()));word(156,1);word(160,0x200);word(0x200+160,0x100);std::memcpy(mdl.data()+0x200+96,id.data(),48);
    check(!readStudioSkinPaletteMetadataCandidate(mdl).linear);word(400,0x300);word(0x310,0x80);word(0x380,1);word(0x384,0x40);word(0x398,0x80);word(0x3c0,0x200);std::memcpy(mdl.data()+0x400,bind.data(),48);
    const auto linear=readStudioSkinPaletteMetadataCandidate(mdl);check(linear.linear&&linear.flags[0]==0x200&&linear.inverseBind[0]==bind);
    word(152,0x10);word(156,UINT32_MAX);word(400,UINT32_MAX);check(readStudioSkinPaletteMetadataCandidate(mdl).staticModel);
    reject([&]{studioSkinPaletteCandidate(m,{},1,{id});});m.staticModel=false;reject([&]{studioSkinPaletteCandidate(m,{id},1,{id});});m.inverseBind[0][0][0]=std::numeric_limits<float>::infinity();reject([&]{studioSkinPaletteCandidate(m,{id,id},0x100,{id,id});});
    word(152,0);reject([&]{readStudioSkinPaletteMetadataCandidate(mdl);});word(156,1);word(400,0x300);word(0x380,2);reject([&]{readStudioSkinPaletteMetadataCandidate(mdl);});word(0x380,1);word(0x398,UINT32_MAX);reject([&]{readStudioSkinPaletteMetadataCandidate(mdl);});
    std::cout<<"PASS checks="<<checks<<" refusals="<<refusals<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
