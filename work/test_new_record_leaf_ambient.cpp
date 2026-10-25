#include "source_new_record_leaf_ambient_candidate.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>

struct ControlledAmbientWorld {
    unsigned lookups=0,traces=0,clips=0;int32_t firstLeaf=0;
    int32_t leafAt(const std::array<float,3>&point){++lookups;return point==std::array<float,3>{1,2,3}?firstLeaf:1;}
    uint32_t contents(int32_t){return 0;}
    void constrainToStartLeaf(const std::array<float,3>&,float,std::array<float,3>&){++clips;}
    SourceLightCacheTraceCandidate trace(const std::array<float,3>&,const std::array<float,3>&,uint32_t mask){if(mask!=0x4081)throw std::runtime_error("trace mask");++traces;return {1,false};}
};
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"x64 required");
    auto require=[](bool value,const char*message){if(!value)throw std::runtime_error(message);};
    require(argc==2,"controlled BSP fixture required");
    std::ifstream f(argv[1],std::ios::binary);require(bool(f),"fixture read");std::vector<uint8_t>b((std::istreambuf_iterator<char>(f)),{});
    SourceBspLeafAmbientCandidate hdr(b,SourceBspAmbientEncodingCandidate::Hdr),ldr(b,SourceBspAmbientEncodingCandidate::Ldr);
    SourceNewRecordLeafAmbientModeCandidate mode{4,false,0,0};
    unsigned checks=0,rejected=0;const std::array<float,3>point{1,2,3};
    for(const auto*data:{&hdr,&ldr})for(int32_t center:{0,1,2,-1,std::numeric_limits<int32_t>::min()}){
        ControlledAmbientWorld world;const auto r=sourceNewRecordLeafAmbientCandidate(point,center,mode,world,*data);
        require(r.inputLeaf==0&&r.queryPoint==point&&r.encoding==data->encoding,"input leaf/point/encoding identity");
        require(r.samplePoint==(center?std::array<float,3>{16,16,64}:point),"new record centering");
        require(world.lookups==(center?2u:1u)&&world.traces==(center?1u:0u)&&world.clips==(center?1u:0u),"leaf requery or zero-mode work");
        require(r.cube[0][0]<(data==&hdr?11.f:2.f),"ambient used centered point's leaf instead of original leaf");
        const auto expected=SourceLeafAmbientInterpolationCandidate::evaluate(*data,0,r.samplePoint);
        require(std::memcmp(expected.data(),r.cube.data(),72)==0,"composed ambient mismatch");++checks;
    }
    auto reject=[&](auto snapshot,auto p,int32_t leaf){ControlledAmbientWorld world;world.firstLeaf=leaf;
        try{(void)sourceNewRecordLeafAmbientCandidate(p,1,snapshot,world,hdr);}catch(const std::exception&){++rejected;require(!world.traces&&!world.clips,"rejection did extra spatial work");return;}
        throw std::runtime_error("unsupported input accepted");};
    for(int32_t radiosity:{0,1,2,3,5,-1}){auto invalid=mode;invalid.radiosity=radiosity;reject(invalid,point,0);}
    auto invalid=mode;invalid.alternateAmbientPath=true;reject(invalid,point,0);
    invalid=mode;invalid.engineState49=1;reject(invalid,point,0);
    invalid=mode;invalid.engineState4A=1;reject(invalid,point,0);
    for(float bad:{INFINITY,-INFINITY,std::numeric_limits<float>::quiet_NaN()})for(unsigned axis=0;axis<3;++axis){auto p=point;p[axis]=bad;reject(mode,p,0);}
    reject(mode,point,-1);reject(mode,point,2);
    // Original compares raw override bytes to1; other nonzero values do not
    // select those override branches. Do not invent boolean normalization.
    mode.engineState49=2;mode.engineState4A=255;ControlledAmbientWorld world;
    (void)sourceNewRecordLeafAmbientCandidate(point,0,mode,world,hdr);++checks;
    std::cout<<"PASS controlled_compositions="<<checks<<" rejections="<<rejected<<" original_leaf_retained=1 zero_mode_no_trace=1 explicit_hdr_ldr=1 raw_override_comparison=1\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
