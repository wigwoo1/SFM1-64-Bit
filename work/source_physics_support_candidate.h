#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <vector>
struct SourcePhysicsSupportCacheCandidate {
    std::array<uint16_t,1024>entries{};
    uint16_t generation=0;
};
struct SourcePhysicsSupportContextCandidate {
    // 0: compact edge walk; 1: contiguous span; 2: transformed packed groups.
    uint32_t mode=0,firstVertex=0;
    std::array<float,9>inverseBasis{}; // Original50h three columns, no padding.
    std::array<float,12>transform{}; // Original18h three Source-space rows.
    std::array<uint16_t,8>seeds{};
    bool seeded=false;
    std::vector<std::array<float,3>>points;
    std::vector<std::array<uint32_t,4>>triangles;
};
struct SourcePhysicsSupportResultCandidate {uint32_t feature=0;std::array<float,3>point{};};
// Original26D40/26BC0 support selection. Inputs are already bound geometry and
// original-prepared transforms/packed points, not a live x86 physics object.
inline SourcePhysicsSupportResultCandidate sourcePhysicsSupportCandidate(
    const SourcePhysicsSupportContextCandidate&context,const std::array<float,3>&direction,
    SourcePhysicsSupportCacheCandidate&cache){
    using V=std::array<float,3>;
    auto finite=[](float x){if(!std::isfinite(x))throw std::runtime_error("nonfinite physics support arithmetic");return x;};
    if(context.mode>2||context.points.empty()||context.points.size()>32768)throw std::runtime_error("invalid support point extent/mode");
    for(float x:direction)finite(x);
    auto dot=[&](const V&p,const V&d){for(float x:p)finite(x);return finite((p[1]*d[1]+p[0]*d[0])+p[2]*d[2]);};
    SourcePhysicsSupportResultCandidate result;
    if(context.mode==2){
        if(context.points.size()%4)throw std::runtime_error("packed support needs complete four-lane groups");
        std::array<float,4>best{};std::array<uint32_t,4>indices{0,1,2,3};
        for(unsigned lane=0;lane<4;++lane)best[lane]=dot(context.points[lane],direction);
        for(size_t group=4;group<context.points.size();group+=4)for(unsigned lane=0;lane<4;++lane){const float value=dot(context.points[group+lane],direction);if(best[lane]<value)indices[lane]=uint32_t(group+lane);best[lane]=value>best[lane]?value:best[lane];}
        // Exact 4E then39 shuffle reduction: ties retain the earlier operand's
        // index. This is a lane priority, not lowest global vertex index.
        const auto oldBest=best;const auto oldIndices=indices;
        for(unsigned lane=0;lane<4;++lane){const unsigned other=(lane+2)%4;if(oldBest[lane]<oldBest[other])indices[lane]=oldIndices[other];best[lane]=oldBest[other]>oldBest[lane]?oldBest[other]:oldBest[lane];}
        const unsigned chosen=best[0]<best[1]?1:0;result.feature=indices[chosen];result.point=context.points[result.feature];return result;
    }
    for(float x:context.inverseBasis)finite(x);for(float x:context.transform)finite(x);
    // Input Source direction -> IVP axes, then original transpose rotation.
    const V ivp{direction[0],-direction[2],direction[1]};V local{};
    for(unsigned component=0;component<3;++component)local[component]=finite((context.inverseBasis[component]*ivp[0]+context.inverseBasis[component+3]*ivp[1])+context.inverseBasis[component+6]*ivp[2]);
    auto nextCache=cache;uint32_t chosen=0;
    if(context.mode==1){
        if(uint64_t(context.firstVertex)+context.points.size()>32768)throw std::runtime_error("support span feature overflow");
        float best=dot(context.points[0],local);for(size_t i=1;i<context.points.size();++i){const float value=dot(context.points[i],local);if(value>best){best=value;chosen=uint32_t(i);}}
        result.feature=context.firstVertex+chosen;
    }else{
        if(context.triangles.empty()||context.triangles.size()>32767)throw std::runtime_error("invalid compact support triangles");
        auto nextEdge=[](size_t edge){return edge/3*3+(edge%3+2)%3;};
        auto word=[&](size_t edge){if(edge>=context.triangles.size()*3)throw std::runtime_error("support edge outside shape");return context.triangles[edge/3][1+edge%3];};
        auto vertex=[&](size_t edge){const uint32_t id=word(edge)&0xffff;if(id>=context.points.size())throw std::runtime_error("support vertex outside shape");return id;};
        auto opposite=[&](size_t edge){const uint32_t raw=word(edge);int32_t offset=int32_t((raw>>16)&0x7fff);if(offset&0x4000)offset-=0x8000;const int64_t physical=int64_t(edge/3*4+1+edge%3)+offset;if(physical<1||physical%4==0||physical>=int64_t(context.triangles.size()*4))throw std::runtime_error("support opposite edge outside shape");return size_t(physical/4*3+physical%4-1);};
        // Validate edge bounds before touching the visit cache. Exact topology
        // traversal below also rejects malformed non-closing rings.
        for(size_t i=0;i<context.triangles.size()*3;++i){vertex(i);opposite(i);}
        size_t edge=0;if(context.seeded){const unsigned octant=(local[0]<0?1:0)|(local[1]<0?2:0)|(local[2]<0?4:0);const uint16_t seed=context.seeds[octant];if((seed&3)>2)throw std::runtime_error("invalid support seed corner");edge=size_t(seed>>2)*3+(seed&3);}
        ++nextCache.generation;if(!nextCache.generation)nextCache.entries.fill(0);
        auto visit=[&](uint32_t id){const size_t slot=(id&511)*2;const bool old=nextCache.entries[slot]==nextCache.generation&&nextCache.entries[slot+1]==uint16_t(id);nextCache.entries[slot]=nextCache.generation;nextCache.entries[slot+1]=uint16_t(id);return old;};
        chosen=vertex(edge);visit(chosen);float best=dot(context.points[chosen],local);
        for(size_t iteration=0;iteration<context.triangles.size();++iteration){
            edge=nextEdge(edge);const uint32_t first=vertex(edge);bool improved=false;std::set<size_t>ring;
            for(;;){
                if(!ring.insert(edge).second)throw std::runtime_error("support neighbor ring does not close");
                const uint32_t id=vertex(edge);if(!visit(id)){const float value=dot(context.points[id],local);if(value>best){chosen=id;best=value;improved=true;break;}}
                edge=nextEdge(opposite(edge));if(vertex(edge)==first)break;
            }
            if(!improved&&first!=chosen)break;
        }
        result.feature=chosen;
    }
    const V p=context.points[chosen];for(float x:p)finite(x);
    for(unsigned row=0;row<3;++row)result.point[row]=finite(((context.transform[row*4+1]*p[1]+context.transform[row*4]*p[0])+context.transform[row*4+2]*p[2])+context.transform[row*4+3]);
    if(context.mode==0)cache=nextCache;
    return result;
}
