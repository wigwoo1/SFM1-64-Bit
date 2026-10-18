#include "source_displacement_core_input_candidate.h"
#include <iostream>
#include <limits>
int main(){try{
 std::vector<std::array<float,5>>raw(9);for(auto&v:raw)v={1,2,3,4,5};std::vector<uint16_t>tags(8,0xbeef);std::vector<std::array<float,20>>extra(9);for(auto&v:extra)for(size_t j=0;j<20;++j)v[j]=float(j+10);
 auto run=[&](uint32_t encoded,uint32_t flags,const auto&e){return sourceDisplacementCoreInputCandidate(1,encoded,flags,0x123,raw,tags,e);};
 auto a=run(0x80000008,0x40000002,std::vector<std::array<float,20>>{});if(a.surfaceFlags!=8||a.flags!=2||a.zeroIndices!=std::vector<uint16_t>(54)||a.triangleTags!=tags)throw std::runtime_error("flags/index/tags");
 for(const auto&v:a.vertices){if(v[0]!=1||v[1]!=2||v[2]!=3||v[3]!=4||v[35]!=5)throw std::runtime_error("raw fields");for(size_t j=4;j<35;++j)if(v[j]!=0)throw std::runtime_error("zero defaults");for(size_t j=36;j<44;++j)if(v[j]!=0)throw std::runtime_error("extra zero defaults");for(size_t j=44;j<56;++j)if(v[j]!=1)throw std::runtime_error("extra one defaults");}
 a=run(7,0x40000002,extra);if(a.surfaceFlags!=0x123||a.flags!=0x40000002)throw std::runtime_error("cached flag retention");for(size_t j=0;j<20;++j)if(a.vertices[0][36+j]!=extra[0][j])throw std::runtime_error("extra copy");a=run(0x80000000,2,extra);if(a.vertices[0][44]!=1||a.vertices[0][36]!=0)throw std::runtime_error("disabled extra copied");
 unsigned rejected=0;auto reject=[&](auto f){try{f();}catch(const std::runtime_error&){++rejected;return;}throw std::runtime_error("invalid input accepted");};
 reject([&]{sourceDisplacementCoreInputCandidate(0,0,0,0,raw,tags);});raw.pop_back();reject([&]{run(0,0,extra);});raw.resize(9);tags.pop_back();reject([&]{run(0,0,extra);});tags.resize(8);extra.pop_back();reject([&]{run(0,0,extra);});extra.resize(9);raw[0][0]=std::numeric_limits<float>::infinity();reject([&]{run(0,0,extra);});raw[0][0]=0;extra[0][0]=std::numeric_limits<float>::quiet_NaN();reject([&]{run(0,0,extra);});if(rejected!=6)throw std::runtime_error("guard count");
 std::cout<<"PASS flags, raw fields, zero/one defaults, tags, extra gates and six invalid-input guards\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
