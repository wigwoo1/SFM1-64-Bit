#pragma once
#include "source_phy_compact_geometry_candidate.h"
#include "source_physics_support_preparation_candidate.h"
// Pointer-free equivalent of original24-byte record: the native ledge pointer
// is replaced by a solid-local ordinal; payload keeps original field offsets.
struct SourcePhysicsSupportMetadataRecordCandidate {
    uint32_t ledge=0;
    std::array<uint8_t,20>payload{};
};
inline SourcePhysicsSupportMetadataRecordCandidate sourcePhysicsSupportMetadataCandidate(
    const SourcePhyConvexGeometryCandidate&convex,uint32_t ledge,
    std::array<uint8_t,20>initial={}){
    const auto&triangles=convex.rawTriangles;const auto&points=convex.rawPoints;
    if(triangles.empty()||triangles.size()>32767||(convex.rawLedgeHeader[3]&0xffff)!=triangles.size())throw std::runtime_error("invalid support metadata triangles");
    SourcePhysicsSupportMetadataRecordCandidate result{ledge,initial};for(unsigned i=0;i<4;++i)result.payload[i]=0;
    auto store16=[&](size_t at,uint16_t value){std::memcpy(result.payload.data()+at,&value,2);};
    if(!(convex.rawLedgeHeader[2]&3)&&triangles.size()<=512){
        uint32_t first=UINT32_MAX,last=0;for(const auto&t:triangles)for(unsigned corner=1;corner<4;++corner){const uint32_t vertex=t[corner]&0xffff;if(vertex<first)first=vertex;if(vertex>last)last=vertex;}
        const size_t span=size_t(last-first)+1;
        if(span<128){
            std::array<bool,128>used{};for(const auto&t:triangles)for(unsigned corner=1;corner<4;++corner)used[(t[corner]&0xffff)-first]=true;
            std::vector<uint8_t>runs;bool current=used[0];uint8_t length=1;for(size_t i=1;i<span;++i){if(used[i]!=current){runs.push_back(length);length=0;current=used[i];}++length;}runs.push_back(length);
            if(runs.size()<=14){
                store16(4,uint16_t(first));
                if(runs.size()==1){result.payload[2]=2;store16(0,runs[0]);}
                else{result.payload[2]=4;result.payload[3]=uint8_t(runs.size());uint16_t count=0;for(size_t i=0;i<runs.size();++i){result.payload[6+i]=runs[i];if(!(i&1))count=uint16_t(count+runs[i]);}store16(0,count);}
                return result;
            }
        }
    }
    // Complete original26310 seed search: no visit-cache optimization. Strict
    // gains and original previous-edge/opposite order determine tied seeds.
    result.payload[2]=1;
    auto vertex=[&](size_t edge){if(edge>=triangles.size()*3)throw std::runtime_error("metadata seed edge outside shape");const uint32_t value=triangles[edge/3][1+edge%3]&0xffff;if(value>=points.size()||value>=32768)throw std::runtime_error("metadata seed vertex outside shape");return value;};
    auto previous=[](size_t edge){return edge/3*3+(edge%3+2)%3;};
    auto opposite=[&](size_t edge){const uint32_t word=triangles[edge/3][1+edge%3];int32_t offset=int32_t((word>>16)&0x7fff);if(offset&0x4000)offset-=0x8000;const int64_t physical=int64_t(edge/3*4+1+edge%3)+offset;if(physical<1||physical%4==0||physical>=int64_t(triangles.size()*4))throw std::runtime_error("metadata seed opposite outside shape");return size_t(physical/4*3+physical%4-1);};
    for(unsigned octant=0;octant<8;++octant){
        const std::array<float,3>d{(octant&1)?-1.f:1.f,(octant&2)?-1.f:1.f,(octant&4)?-1.f:1.f};
        auto dot=[&](uint32_t id){std::array<float,3>p;std::memcpy(p.data(),points[id].data(),12);for(float x:p)if(!std::isfinite(x))throw std::runtime_error("nonfinite metadata seed point");const float value=(p[1]*d[1]+p[0]*d[0])+p[2]*d[2];if(!std::isfinite(value))throw std::runtime_error("metadata seed projection overflow");return value;};
        size_t edge=0,chosenTriangle=0;uint32_t chosen=vertex(edge);float best=dot(chosen);
        for(size_t iteration=0;iteration<triangles.size();++iteration){
            edge=previous(edge);const uint32_t first=vertex(edge);bool improved=false;std::set<size_t>ring;
            for(;;){
                if(!ring.insert(edge).second)throw std::runtime_error("metadata seed neighbor ring does not close");const uint32_t id=vertex(edge);const float value=dot(id);
                if(value>best){best=value;chosen=id;chosenTriangle=edge/3;improved=true;break;}
                edge=previous(opposite(edge));if(vertex(edge)==first)break;
            }
            if(!improved&&first!=chosen)break;
        }
        unsigned corner=0;for(unsigned i=0;i<3;++i)if((triangles[chosenTriangle][1+i]&0xffff)==chosen){corner=i;break;}
        store16(4+octant*2,uint16_t(chosenTriangle*4+corner)); // Original stores AX.
    }
    return result;
}
inline std::vector<SourcePhysicsSupportMetadataRecordCandidate>sourcePhysicsSupportMetadataTableCandidate(
    const std::vector<SourcePhyConvexGeometryCandidate>&solid){
    std::vector<SourcePhysicsSupportMetadataRecordCandidate>result;
    if(solid.empty()||solid.size()>32)return result;
    for(size_t i=0;i<solid.size();++i)result.push_back(sourcePhysicsSupportMetadataCandidate(solid[i],uint32_t(i)));
    return result;
}
