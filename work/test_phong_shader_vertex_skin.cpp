#include "phong_shader_vertex_skin_candidate.h"
#include <fstream>
#include <iostream>
#include <limits>
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"x64 required");auto require=[](bool b,const char*s){if(!b)throw std::runtime_error(s);};unsigned checks=0,refusals=0;
    std::array<GenericShaderSkinMatrixCandidate,3>m{};for(auto&a:m)for(unsigned i=0;i<3;++i)a[i][i]=1;
    std::array<uint8_t,8>w{};float one=1;std::memcpy(w.data(),&one,4);
    std::array<uint8_t,12>n{};std::memcpy(n.data()+8,&one,4);
    std::array<uint8_t,16>t{};std::memcpy(t.data(),&one,4);std::memcpy(t.data()+12,&one,4);
    for(float sign:{-1.0f,1.0f}){std::memcpy(t.data()+12,&sign,4);
        auto a=phongShaderSkinVertexCandidate(m,false,w,n,t,{1,2,3},{2,3,4},{0,0,0},0);
        require(a.position==std::array<float,3>{1,2,3}&&a.normal==std::array<float,3>{0,0,1}&&a.tangent==std::array<float,3>{1,0,0},"identity frame");++checks;
        require(a.binormal[0]==0&&a.binormal[1]==sign&&a.binormal[2]==0,"cross orientation/handedness");++checks;
    }
    auto flex=phongShaderSkinVertexCandidate(m,false,w,n,t,{1,2,3},{2,3,4},{0,1,0},1);
    require(flex.position==std::array<float,3>{3,5,7}&&flex.normal[1]>0&&flex.tangent[1]>0,"shared direction flex");++checks;
    auto translated=m;for(auto&a:translated){a[0][3]=100;a[1][3]=200;a[2][3]=300;}
    auto a=phongShaderSkinVertexCandidate(translated,false,w,n,t,{1,2,3},{0,0,0},{0,0,0},0);
    require(a.position==std::array<float,3>{101,202,303}&&a.normal==std::array<float,3>{0,0,1}&&a.tangent==std::array<float,3>{1,0,0},"direction must ignore translation");++checks;
    auto reject=[&](auto action,bool overflow=false){try{action();}catch(const std::invalid_argument&){require(!overflow,"wrong refusal");++refusals;return;}catch(const std::overflow_error&){require(overflow,"unexpected overflow");++refusals;return;}throw std::runtime_error("invalid Phong input accepted");};
    const float nan=std::numeric_limits<float>::quiet_NaN(),inf=std::numeric_limits<float>::infinity();
    reject([&]{phongShaderSkinVertexCandidate(m,false,w,n,t,{0,0,0},{0,0,0},{0,0,0},inf);});
    reject([&]{auto q=t;std::memcpy(q.data()+12,&nan,4);phongShaderSkinVertexCandidate(m,false,w,n,q,{0,0,0},{0,0,0},{0,0,0},0);});
    reject([&]{auto q=t;float zero=0;std::memcpy(q.data()+12,&zero,4);phongShaderSkinVertexCandidate(m,false,w,n,q,{0,0,0},{0,0,0},{0,0,0},0);});
    reject([&]{auto q=t;std::memcpy(q.data(),n.data(),12);phongShaderSkinVertexCandidate(m,false,w,n,q,{0,0,0},{0,0,0},{0,0,0},0);});
    reject([&]{auto q=t;std::memset(q.data(),0,12);phongShaderSkinVertexCandidate(m,false,w,n,q,{0,0,0},{0,0,0},{0,0,0},0);});
    reject([&]{auto q=n;std::memcpy(q.data(),&nan,4);phongShaderSkinVertexCandidate(m,false,w,q,t,{0,0,0},{0,0,0},{0,0,0},0);});
    reject([&]{phongShaderSkinVertexCandidate(m,false,w,n,t,{nan,0,0},{0,0,0},{0,0,0},0);});
    reject([&]{phongShaderSkinVertexCandidate(m,false,w,n,t,{0,0,0},{inf,0,0},{0,0,0},0);});
    reject([&]{phongShaderSkinVertexCandidate(m,false,w,n,t,{0,0,0},{0,0,0},{nan,0,0},0);});
    reject([&]{auto q=w;std::memcpy(q.data(),&nan,4);phongShaderSkinVertexCandidate(m,false,q,n,t,{0,0,0},{0,0,0},{0,0,0},0);});
    reject([&]{auto q=m;q[0][0][0]=inf;phongShaderSkinVertexCandidate(q,false,w,n,t,{0,0,0},{0,0,0},{0,0,0},0);});
    reject([&]{auto q=m;for(auto&matrix:q)matrix[0][0]=2;phongShaderSkinVertexCandidate(q,false,w,n,t,{std::numeric_limits<float>::max(),0,0},{0,0,0},{0,0,0},0);},true);
    require(argc==3,"decode and vertex fixtures required");std::ifstream decode(argv[1],std::ios::binary),skin(argv[2],std::ios::binary);require(bool(decode)&&bool(skin),"fixture open");
    std::array<uint8_t,28>d{};size_t decodes=0;
    while(decode.read(reinterpret_cast<char*>(d.data()),28)){const auto f=phongDecodePackedFrameCandidate({d[0],d[1],d[2],d[3]});
        require(std::memcmp(f.normal.data(),d.data()+4,12)==0&&std::memcmp(f.tangent.data(),d.data()+16,12)==0,"Phong token frame decode differs");++decodes;}
    require(decode.eof()&&!decode.gcount()&&decodes,"decode fixture framing");
    std::array<uint8_t,272>q{};size_t vertices=0,degenerate=0;
    while(skin.read(reinterpret_cast<char*>(q.data()),272)){
        uint32_t profile=0;std::memcpy(&profile,q.data(),4);require((profile&~0x100u)<4,"profile fixture");
        std::array<GenericShaderSkinMatrixCandidate,3>matrices{};std::memcpy(matrices.data(),q.data()+4,144);
        std::array<uint8_t,8>weights{};std::array<uint8_t,12>normal{};std::array<uint8_t,16>tangent{};
        std::memcpy(weights.data(),q.data()+148,8);std::memcpy(normal.data(),q.data()+156,12);std::memcpy(tangent.data(),q.data()+168,16);
        std::array<float,3>p{},pd{},nd{};float scale=0;std::memcpy(p.data(),q.data()+184,12);std::memcpy(pd.data(),q.data()+196,12);std::memcpy(nd.data(),q.data()+208,12);std::memcpy(&scale,q.data()+220,4);
        if(profile&0x100u){try{phongShaderSkinVertexCandidate(matrices,(profile&1)!=0,weights,normal,tangent,p,pd,nd,scale);}catch(const std::invalid_argument&){++degenerate;continue;}throw std::runtime_error("degenerate frame accepted");}
        const auto f=phongShaderSkinVertexCandidate(matrices,(profile&1)!=0,weights,normal,tangent,p,pd,nd,scale);
        require(std::memcmp(f.position.data(),q.data()+224,12)==0&&std::memcmp(f.normal.data(),q.data()+236,12)==0&&std::memcmp(f.tangent.data(),q.data()+248,12)==0&&std::memcmp(f.binormal.data(),q.data()+260,12)==0,"Phong assembled token frame differs");++vertices;
    }
    require(skin.eof()&&!skin.gcount()&&vertices,"vertex fixture framing");
    std::cout<<"PASS checks="<<checks<<" refusals="<<refusals<<" packed_frame_cases="<<decodes<<" vertex_frame_cases="<<vertices<<" degenerate_frame_refusals="<<degenerate<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
