#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include "source_bsp_static_props_candidate.h"
template<class T>static void put(std::ostream&o,const T&v){o.write(reinterpret_cast<const char*>(&v),sizeof(v));}
template<class T>static T at(const std::vector<uint8_t>&b,size_t p){T v;std::memcpy(&v,b.data()+p,sizeof(v));return v;}
template<class T>static void set(std::vector<uint8_t>&b,size_t p,T v){std::memcpy(b.data()+p,&v,sizeof(v));}
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"owned x64");if(argc!=3)throw std::runtime_error("BSP and record output required");std::ifstream f(argv[1],std::ios::binary);if(!f)throw std::runtime_error("BSP read");const std::vector<uint8_t>b((std::istreambuf_iterator<char>(f)),{});const auto before=b;const SourceBspStaticPropsCandidate props(b);std::ofstream out(argv[2],std::ios::binary);if(!out)throw std::runtime_error("records output");
    put(out,uint32_t(props.models().size()));for(const auto&name:props.models()){put(out,uint32_t(name.size()));out.write(name.data(),name.size());}put(out,uint32_t(props.leaves().size()));for(auto leaf:props.leaves())put(out,leaf);put(out,uint32_t(props.records().size()));
    for(const auto&r:props.records()){put(out,r.origin);put(out,r.angles);put(out,r.model);put(out,r.firstLeaf);put(out,r.leafCount);put(out,r.solidType);put(out,r.propFlags);put(out,r.skin);put(out,r.fadeMin);put(out,r.fadeMax);put(out,r.lightingOrigin);put(out,r.forcedFadeScale);}
    out.flush();if(!out)throw std::runtime_error("record output failure");size_t checks=0;
    auto require=[&](bool ok){if(!ok)throw std::runtime_error("static prop safety check failed");++checks;};
    auto reject=[&](const std::vector<uint8_t>&bad){const auto snapshot=bad;bool failed=false;try{const SourceBspStaticPropsCandidate ignored(bad);}catch(const std::runtime_error&){failed=true;}require(failed&&snapshot==bad);};
    const size_t game=at<uint32_t>(b,8+35*16);size_t entry=0;for(size_t i=0;i<at<uint32_t>(b,game);++i){const size_t p=game+4+i*16;if(at<uint32_t>(b,p)==0x73707270u)entry=p;}require(entry!=0);const size_t off=at<uint32_t>(b,entry+8),modelCount=at<uint32_t>(b,off),leafCountAt=off+4+modelCount*128,leafCount=at<uint32_t>(b,leafCountAt),countAt=leafCountAt+4+leafCount*2,first=countAt+4;require(!props.records().empty());
    auto mutate=[&](size_t p,auto value){auto bad=b;set(bad,p,value);reject(bad);};
    reject({});for(size_t n:{size_t(4),size_t(1035),game,off,leafCountAt,countAt,first}){auto bad=b;bad.resize(n);reject(bad);}
    mutate(0,uint32_t(0));mutate(4,uint32_t(21));mutate(8+10*16+8,uint32_t(0));mutate(8+10*16+4,uint32_t(31));mutate(8+35*16+12,uint32_t(1));mutate(8+35*16+8,uint32_t(1));mutate(game,UINT32_MAX);mutate(entry+4,uint16_t(1));mutate(entry+6,uint16_t(6));mutate(entry+8,uint32_t(0));mutate(entry+8,UINT32_MAX);mutate(entry+12,UINT32_MAX);mutate(off,UINT32_MAX);mutate(leafCountAt,UINT32_MAX);mutate(countAt,uint32_t(65537));mutate(countAt,uint32_t(props.records().size()-1));mutate(first+24,uint16_t(modelCount));mutate(first+26,uint16_t(65535));mutate(first+28,uint16_t(65535));mutate(leafCountAt+4,uint16_t(65535));mutate(first+30,uint8_t(5));
    for(size_t p:{size_t(0),size_t(12),size_t(36),size_t(40),size_t(56)})mutate(first+p,uint32_t(0x7fc00000));
    {auto bad=b;bad[first+31]|=2;set(bad,first+44,uint32_t(0x7fc00000));reject(bad);}
    // Original101C50E5 reads lightingOrigin ONLY with propFlags bit2. The
    // benchmark contains inactive NaN bit patterns here; preserve them.
    {auto valid=b;valid[first+31]&=0xfdu;set(valid,first+44,uint32_t(0x7fc00000));const SourceBspStaticPropsCandidate accepted(valid);require(std::isnan(accepted.records()[0].lightingOrigin[0]));}
    {auto bad=b;std::memset(bad.data()+off+4,'a',128);reject(bad);}mutate(off+4,uint8_t(0));mutate(off+4,uint8_t(':'));{auto bad=b;std::memcpy(bad.data()+off+4,"models/../bad.mdl",17);reject(bad);}
    {auto bad=b;const size_t other=entry==game+4?entry+16:game+4;std::memcpy(bad.data()+other,bad.data()+entry,16);reject(bad);}
    for(uint8_t solid:{uint8_t(0),uint8_t(2),uint8_t(6)}){auto r=props.records()[0];r.origin={-0.f,1.f,-2.f};r.angles={-0.f,90.f,15.f};r.propFlags=255;r.solidType=solid;const auto input=r;for(size_t index:{size_t(0),size_t(65535)}){const auto v=sourceStaticPropCollisionBindingCandidate(r,index,0x1234567887654321ull);require(v.referenceHandle==(0x40000000u|uint32_t(index))&&v.modelId==0x1234567887654321ull&&v.solidFlags==0&&v.solidType==solid);require(!std::memcmp(v.origin.data(),r.origin.data(),12));const std::array<float,3>zero{};require(!std::memcmp(v.angles.data(),solid==6?r.angles.data():zero.data(),12));}require(!std::memcmp(input.origin.data(),r.origin.data(),12)&&input.propFlags==r.propFlags);}
    auto invalidBinding=[&](SourceBspStaticPropRecordCandidate r,size_t index,uint64_t model){bool failed=false;try{const auto v=sourceStaticPropCollisionBindingCandidate(r,index,model);(void)v;}catch(const std::runtime_error&){failed=true;}require(failed);};
    invalidBinding(props.records()[0],65536,1);invalidBinding(props.records()[0],0,0);{auto r=props.records()[0];r.solidType=5;invalidBinding(r,0,1);}{auto r=props.records()[0];r.origin[0]=std::numeric_limits<float>::infinity();invalidBinding(r,0,1);}{auto r=props.records()[0];r.angles[1]=std::numeric_limits<float>::quiet_NaN();invalidBinding(r,0,1);}
    require(b==before);std::cout<<"PASS static prop safety checks="<<checks<<" records="<<props.records().size()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
