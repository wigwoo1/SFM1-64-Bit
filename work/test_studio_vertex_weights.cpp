#include "studio_vertex_weights_candidate.h"
#include <iostream>
#include <limits>
int main(){try{
    static_assert(sizeof(void*)==8,"x64 required");
    auto require=[](bool b,const char*s){if(!b)throw std::runtime_error(s);};unsigned checks=0,refusals=0;
    std::array<uint8_t,48>vertex{};std::array<uint8_t,9>opt{1,0,2,3,0,0,52,255,26};
    const std::array<float,3>w{0.25f,0.5f,0.25f};std::memcpy(vertex.data(),w.data(),12);vertex[15]=3;
    std::array<uint8_t,8>initial{};initial.fill(0xa5);
    auto result=studioVertexWeightsCandidate(vertex,opt,false,2,initial);std::array<float,2>decoded{};std::memcpy(decoded.data(),result.weights.data(),8);
    require(result.valid&&decoded==std::array<float,2>{0.5f,0.25f}&&result.indices==std::array<uint8_t,4>{52,0,26,0},"weight reorder/255 remap");++checks;
    result=studioVertexWeightsCandidate(vertex,opt,true,2,initial);uint32_t packed=0;std::memcpy(&packed,result.weights.data(),4);
    require(packed==0x1fff3fff&&result.weights[4]==0xa5,"compressed residual/extent");++checks;
    result=studioVertexWeightsCandidate(vertex,opt,true,1,initial);std::memcpy(&packed,result.weights.data(),4);require(packed==0x3fff,"one stored weight extent");++checks;
    opt[3]=0;for(bool compressed:{false,true}){result=studioVertexWeightsCandidate(vertex,opt,compressed,2,initial);require(result.valid&&result.weights==initial&&result.indices==std::array<uint8_t,4>{},"zero optimized bones writes weights");++checks;}
    opt[3]=3;std::array<float,3>badSum{0.2f,0.2f,0.2f};std::memcpy(vertex.data(),badSum.data(),12);result=studioVertexWeightsCandidate(vertex,opt,false,2,initial);std::memcpy(decoded.data(),result.weights.data(),8);require(!result.valid&&decoded==std::array<float,2>{0.2f,0.2f},"invalid sum unexpectedly normalized");++checks;
    auto rejected=[&](const auto&v,const auto&o,unsigned n){try{studioVertexWeightsCandidate(v,o,true,n,initial);}catch(const std::invalid_argument&){++refusals;return;}throw std::runtime_error("malformed accepted");};
    for(uint8_t n:{uint8_t(0),uint8_t(4)}){auto v=vertex;v[15]=n;rejected(v,opt,2);}
    for(unsigned n:{0u,3u})rejected(vertex,opt,n);
    auto o=opt;o[3]=4;rejected(vertex,o,2);o=opt;o[0]=3;rejected(vertex,o,2);o=opt;o[6]=53;rejected(vertex,o,2);
    for(float v:{-0.1f,1.1f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){auto bad=vertex;std::memcpy(bad.data(),&v,4);rejected(bad,opt,2);}
    std::cout<<"PASS checks="<<checks<<" refusals="<<refusals<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
