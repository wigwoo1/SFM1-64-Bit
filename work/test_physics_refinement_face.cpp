#include "source_physics_refinement_face_candidate.h"
#include <cstring>
#include <iostream>
#include <limits>
int main(){try{
    unsigned checks=0;auto require=[&](bool pass){if(!pass)throw std::runtime_error("refinement face assertion");++checks;};SourcePhysicsSimplexCandidate s;s.vertices[0].point={1,0,0};s.vertices[1].point={0,1,0};s.vertices[2].point={0,0,1};s.vertices[3].point={0,0,0};s.count=0xffffffff;for(unsigned j=0;j<4;++j){s.vertices[j].featureA=uint16_t(0x8100+j);s.vertices[j].featureB=uint16_t(0xf200+j);}const auto initial=s;
    const std::array<std::array<float,3>,3>directions{{{{0,0,1}},{{1,0,0}},{{0,1,0}}}};
    for(bool includeBase:{false,true})for(unsigned j=0;j<3;++j){s=initial;const auto normal=sourcePhysicsRefinementFaceCandidate(s,directions[j],includeBase);require(normal==directions[j]);require(s.count==3);const unsigned moved=j==0?2:(j==1?0:1);require(std::memcmp(&s.vertices[moved],&initial.vertices[3],16)==0);for(unsigned k=0;k<4;++k)if(k!=moved)require(std::memcmp(&s.vertices[k],&initial.vertices[k],16)==0);}
    s=initial;const auto base=sourcePhysicsRefinementFaceCandidate(s,{-1,-1,-1},true);require(base==std::array<float,3>{-1,-1,-1});require(std::memcmp(&s.vertices[0],&initial.vertices[0],16)==0);require(std::memcmp(&s.vertices[1],&initial.vertices[2],16)==0);require(std::memcmp(&s.vertices[2],&initial.vertices[1],16)==0);require(std::memcmp(&s.vertices[3],&initial.vertices[1],16)==0);
    s=initial;const auto update=sourcePhysicsRefinementFaceCandidate(s,{-1,-1,-1},false);require(update==std::array<float,3>{0,1,0});require(std::memcmp(&s.vertices[1],&initial.vertices[3],16)==0);
    for(bool includeBase:{false,true}){s=initial;sourcePhysicsRefinementFaceCandidate(s,{0,0,0},includeBase);require(std::memcmp(&s.vertices[includeBase?3:1],&initial.vertices[includeBase?1:3],16)==0);}
    for(bool includeBase:{false,true})for(unsigned variant=0;variant<3;++variant){s=initial;std::array<float,3>d{1,2,3};if(variant==0)s.vertices[3].point[0]=std::numeric_limits<float>::quiet_NaN();if(variant==1)d[2]=std::numeric_limits<float>::infinity();if(variant==2){s.vertices[0].point[0]=3e38f;s.vertices[1].point[1]=3e38f;}const auto before=s;bool rejected=false;try{sourcePhysicsRefinementFaceCandidate(s,d,includeBase);}catch(const std::runtime_error&){rejected=true;}require(rejected);require(std::memcmp(&s,&before,sizeof(s))==0);}
    std::cout<<"PASS refinement-face analytical/transaction checks="<<checks<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
