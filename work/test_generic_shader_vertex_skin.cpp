#include "generic_shader_vertex_skin_candidate.h"
#include <fstream>
#include <iostream>
#include <limits>
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"x64 required");auto require=[](bool b,const char*s){if(!b)throw std::runtime_error(s);};unsigned checks=0,refusals=0;
    const std::array<std::array<float,3>,6>axes{{{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}}};
    for(const auto&axis:axes){const auto packed=genericPackNormalCandidate(axis);require(genericDecodeNormalCandidate({packed[0],packed[1]})==axis,"axis pack/decode");++checks;}
    for(const auto&bad:std::array<std::array<float,3>,4>{{{0,0,0},{2,0,0},{std::numeric_limits<float>::infinity(),0,0},{std::numeric_limits<float>::quiet_NaN(),0,0}}}){
        try{genericPackNormalCandidate(bad);}catch(const std::invalid_argument&){++refusals;continue;}throw std::runtime_error("bad pack normal accepted");}
    std::array<GenericShaderSkinMatrixCandidate,3>identity{};for(auto&m:identity)for(unsigned i=0;i<3;++i)m[i][i]=1;
    std::array<uint8_t,8>weight{};float one=1;std::memcpy(weight.data(),&one,4);std::array<uint8_t,12>normal{};std::memcpy(normal.data()+8,&one,4);
    const auto direct=genericShaderSkinVertexCandidate(identity,false,weight,normal,{1,2,3},{0,0,0},{0,0,0},0,true,1);
    require(direct.position==std::array<float,3>{1,2,3}&&direct.normal==axes[4],"identity skin");++checks;
    const auto gated=genericShaderSkinVertexCandidate(identity,false,weight,normal,{1,2,3},{0,0,0},{0,1,0},1,false,1e-30f);
    require(gated.normal==axes[2],"static predicate underflow or missing flex");++checks;
    auto reject=[&](auto action,bool overflow=false){try{action();}catch(const std::invalid_argument&){require(!overflow,"wrong overflow refusal");++refusals;return;}catch(const std::overflow_error&){require(overflow,"unexpected overflow");++refusals;return;}throw std::runtime_error("invalid skin accepted");};
    reject([&]{genericShaderSkinVertexCandidate(identity,false,weight,normal,{0,0,0},{0,0,0},{0,0,0},std::numeric_limits<float>::infinity(),true,1);});
    reject([&]{genericShaderSkinVertexCandidate(identity,false,weight,normal,{0,0,0},{0,0,0},{0,0,0},0,true,std::numeric_limits<float>::quiet_NaN());});
    reject([&]{auto w=weight;float bad=std::numeric_limits<float>::quiet_NaN();std::memcpy(w.data(),&bad,4);genericShaderSkinVertexCandidate(identity,false,w,normal,{0,0,0},{0,0,0},{0,0,0},0,true,1);});
    reject([&]{std::array<GenericShaderSkinMatrixCandidate,3>zero{};genericShaderSkinVertexCandidate(zero,false,weight,normal,{0,0,0},{0,0,0},{0,0,0},0,true,1);});
    reject([&]{genericShaderSkinVertexCandidate(identity,false,weight,normal,{0,0,0},{0,0,0},{0,0,0},0,false,0);});
    reject([&]{genericShaderSkinVertexCandidate(identity,false,weight,normal,{std::numeric_limits<float>::quiet_NaN(),0,0},{0,0,0},{0,0,0},0,true,1);});
    reject([&]{auto m=identity;for(auto&matrix:m)matrix[0][0]=2;genericShaderSkinVertexCandidate(m,false,weight,normal,{std::numeric_limits<float>::max(),0,0},{0,0,0},{0,0,0},0,true,1);},true);
    require(argc==3,"decode and skin fixtures required");std::ifstream decode(argv[1],std::ios::binary),skin(argv[2],std::ios::binary);require(bool(decode)&&bool(skin),"fixture open");
    std::array<uint8_t,16>d{};size_t decodes=0;
    while(decode.read(reinterpret_cast<char*>(d.data()),16)){const auto n=genericDecodeNormalCandidate({d[0],d[1]});require(std::memcmp(n.data(),d.data()+4,12)==0,"normal token decode differs");++decodes;}
    require(decode.eof()&&!decode.gcount()&&decodes,"normal fixture framing");
    std::array<uint8_t,236>q{};size_t vertices=0;
    while(skin.read(reinterpret_cast<char*>(q.data()),236)){
        uint32_t variant=0;std::memcpy(&variant,q.data(),4);require(variant>=4&&variant<=7,"skin fixture variant");std::array<GenericShaderSkinMatrixCandidate,3>matrices{};std::memcpy(matrices.data(),q.data()+4,144);
        std::array<uint8_t,8>w{};std::array<uint8_t,12>n{};std::memcpy(w.data(),q.data()+148,8);std::memcpy(n.data(),q.data()+156,12);
        std::array<float,3>p{},pd{},nd{};float scale=0;std::memcpy(p.data(),q.data()+168,12);std::memcpy(pd.data(),q.data()+180,12);std::memcpy(nd.data(),q.data()+192,12);std::memcpy(&scale,q.data()+204,4);
        float staticLight=0;std::memcpy(&staticLight,q.data()+208,4);
        const auto result=genericShaderSkinVertexCandidate(matrices,(variant&1)!=0,w,n,p,pd,nd,scale,(variant&2)!=0,staticLight);
        require(std::memcmp(result.position.data(),q.data()+212,12)==0&&std::memcmp(result.normal.data(),q.data()+224,12)==0,"assembled shader token reference differs");++vertices;
    }
    require(skin.eof()&&!skin.gcount()&&vertices,"skin fixture framing");std::cout<<"PASS checks="<<checks<<" refusals="<<refusals<<" normal_decode_cases="<<decodes<<" skin_vertex_cases="<<vertices<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
