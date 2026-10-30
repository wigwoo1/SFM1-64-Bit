// Test-only ABI for the explicit PS20b static0/dynamic0 ambient branch.
// No editor integration; no material or hardware capability inference.
#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vector>
struct PhongAmbientPixelQuery {
    std::array<float,4> base,normalSample,tangent,binormal,normal,position;
    std::array<float,4> diffuse,eye,fog,controls,fogColor,lightScale;
    std::array<std::array<float,4>,6> cube;
};
static_assert(sizeof(PhongAmbientPixelQuery)==288,"fixture ABI");
inline std::vector<PhongAmbientPixelQuery> phongAmbientPixelQueries(const char*path){
    std::ifstream f(path,std::ios::binary|std::ios::ate);
    if(!f)throw std::runtime_error("missing query file");
    const auto n=f.tellg();
    if(n<=0||uint64_t(n)>4*1024*1024||uint64_t(n)%sizeof(PhongAmbientPixelQuery))
        throw std::runtime_error("query framing/size");
    std::vector<PhongAmbientPixelQuery> q(size_t(n)/sizeof(PhongAmbientPixelQuery));
    f.seekg(0);f.read(reinterpret_cast<char*>(q.data()),n);
    if(!f)throw std::runtime_error("query read");
    for(const auto&r:q){
        std::array<float,72> words{};std::memcpy(words.data(),&r,sizeof(r));
        for(float v:words)if(!std::isfinite(v)||std::abs(v)>64)throw std::runtime_error("query finite bounded domain");
        for(unsigned i=0;i<4;++i)if(r.base[i]<0||r.base[i]>1||r.normalSample[i]<0||r.normalSample[i]>1)
            throw std::runtime_error("query sampled texture domain");
        double length=0,distance=0;
        for(unsigned i=0;i<3;++i){
            const double v=r.tangent[i]*(2.0*r.normalSample[0]-1)+r.binormal[i]*(2.0*r.normalSample[1]-1)+r.normal[i]*(2.0*r.normalSample[2]-1);
            length+=v*v;const double d=double(r.eye[i])-r.position[i];distance+=d*d;
        }
        if(length<1e-8||distance<1e-8)throw std::runtime_error("query degenerate normal/eye distance");
        if(r.fog[2]<0||r.fog[2]>1)throw std::runtime_error("query fog cap domain");
    }
    return q;
}
