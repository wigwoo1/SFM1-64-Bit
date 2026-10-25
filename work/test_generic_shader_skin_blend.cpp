#include "generic_shader_skin_blend_candidate.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"x64 required");
    auto require=[](bool good,const char*why){if(!good)throw std::runtime_error(why);};
    std::array<GenericShaderSkinMatrixCandidate,3> matrices{};
    for(auto&row:matrices[0])row.fill(2);
    for(auto&row:matrices[1])row.fill(4);
    for(auto&row:matrices[2])row.fill(8);
    unsigned analytical=0,refusals=0;
    for(const auto&w:std::array<std::array<float,2>,4>{{{1,0},{0,1},{0,0},{1,1}}}){
        auto result=genericShaderSkinBlendCandidate(matrices,w);
        const float expected=w[0]==1?(w[1]==1?-2.0f:2.0f):(w[1]==1?4.0f:8.0f);
        for(const auto&row:result)for(float v:row)require(v==expected,"basis/residual or unwanted weight clamp");++analytical;
    }
    for(float bad:{std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){
        for(size_t i=0;i<2;++i){std::array<float,2>w{0.25f,0.25f};w[i]=bad;
            try{genericShaderSkinBlendCandidate(matrices,w);}catch(const std::invalid_argument&){++refusals;continue;}throw std::runtime_error("bad weight accepted");}
        for(size_t b=0;b<3;++b)for(size_t r=0;r<3;++r)for(size_t c=0;c<4;++c){auto m=matrices;m[b][r][c]=bad;
            try{genericShaderSkinBlendCandidate(m,{0.25f,0.25f});}catch(const std::invalid_argument&){++refusals;continue;}throw std::runtime_error("bad matrix accepted");}
    }
    try{genericShaderSkinBlendCandidate(matrices,{std::numeric_limits<float>::max(),std::numeric_limits<float>::max()});throw std::runtime_error("overflow accepted");}
    catch(const std::overflow_error&){++refusals;}
    require(argc==2,"fixture argument required");std::ifstream input(argv[1],std::ios::binary);require(bool(input),"fixture open");
    std::array<float,50> record{};size_t cases=0;
    while(input.read(reinterpret_cast<char*>(record.data()),sizeof(record))){
        std::memcpy(matrices.data(),record.data(),144);const auto before=matrices;
        const auto result=genericShaderSkinBlendCandidate(matrices,{record[36],record[37]});
        require(std::memcmp(result.data(),record.data()+38,48)==0,"token reference matrix mismatch");
        require(std::memcmp(before.data(),matrices.data(),144)==0,"inputs modified");++cases;
    }
    require(input.eof()&&input.gcount()==0&&cases>0,"empty or partial fixture");
    std::cout<<"PASS analytical="<<analytical<<" refusals="<<refusals<<" cases="<<cases<<" x64=1\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
