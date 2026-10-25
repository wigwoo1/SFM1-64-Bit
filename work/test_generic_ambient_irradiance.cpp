#include "generic_ambient_irradiance_candidate.h"
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>

int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"Native x64 required");
    auto require=[](bool good,const char*message){if(!good)throw std::runtime_error(message);};
    GenericAmbientCubeCandidate cube{};
    for(unsigned face=0;face<6;++face)cube[face]={float(face+1),float(face+11),float(face+21)};
    unsigned analytical=0;
    for(unsigned axis=0;axis<3;++axis)for(unsigned sign=0;sign<2;++sign){
        std::array<float,3>normal{};normal[axis]=sign?-1.0f:1.0f;
        require(genericAmbientIrradianceCandidate(normal,cube)==cube[axis*2+sign],"axis selects wrong face");++analytical;
    }
    require(genericAmbientIrradianceCandidate({0.0f,-0.0f,0.0f},cube)==std::array<float,3>{0,0,0},"zero expression");++analytical;
    const auto unnormalized=genericAmbientIrradianceCandidate({2,0,0},cube);
    require(unnormalized==std::array<float,3>{4,44,84},"unexpected normalization or output clamp");++analytical;
    for(auto&face:cube)face={-2,4,8};
    require(genericAmbientIrradianceCandidate({0.5f,-0.5f,0.5f},cube)==std::array<float,3>{-1.5f,3,6},"mixed axes or unwanted clamp");++analytical;
    unsigned rejected=0;
    for(float bad:{std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){
        for(unsigned axis=0;axis<3;++axis){std::array<float,3>n{1,0,0};n[axis]=bad;
            try{genericAmbientIrradianceCandidate(n,cube);}catch(const std::invalid_argument&){++rejected;continue;}
            throw std::runtime_error("nonfinite normal accepted");}
        for(unsigned face=0;face<6;++face)for(unsigned channel=0;channel<3;++channel){auto invalid=cube;invalid[face][channel]=bad;
            try{genericAmbientIrradianceCandidate({1,0,0},invalid);}catch(const std::invalid_argument&){++rejected;continue;}
            throw std::runtime_error("nonfinite cube accepted");}
    }
    try{genericAmbientIrradianceCandidate({std::numeric_limits<float>::max(),0,0},cube);throw std::runtime_error("overflow accepted");}
    catch(const std::overflow_error&){++rejected;}
    require(argc==2,"reference fixture required");
    std::ifstream input(argv[1],std::ios::binary);require(bool(input),"fixture open");
    std::array<float,24>record{};size_t cases=0;
    while(input.read(reinterpret_cast<char*>(record.data()),sizeof(record))){
        std::array<float,3>normal{};std::memcpy(normal.data(),record.data(),12);
        GenericAmbientCubeCandidate colors{};std::memcpy(colors.data(),record.data()+3,72);
        const auto beforeNormal=normal;const auto beforeCube=colors;
        const auto result=genericAmbientIrradianceCandidate(normal,colors);
        require(std::memcmp(result.data(),record.data()+21,12)==0,"bytecode-reference expression differs");
        require(normal==beforeNormal&&colors==beforeCube,"inputs modified");++cases;
    }
    require(input.eof()&&input.gcount()==0&&cases>0,"truncated or empty fixture");
    std::cout<<"PASS analytical="<<analytical<<" guard_rejections="<<rejected<<" bytecode_reference_cases="<<cases<<" x64=1\n";
    return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
