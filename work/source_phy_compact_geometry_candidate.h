#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>
struct SourcePhyConvexGeometryCandidate {
    uint32_t gameData=0;
    std::vector<std::array<float,9>>triangles;
    // Optional lossless collision data. These are IVP metre/axis bit patterns,
    // not the converted Source-unit triangles above. Relative references are
    // retained as file-format data and are never cast to native pointers.
    std::array<uint32_t,4>rawLedgeHeader{};
    std::vector<std::array<uint32_t,4>>rawTriangles,rawPoints;
};
struct SourcePhyCollisionTreeNodeCandidate {
    std::array<uint32_t,7>raw{};
    uint32_t left=UINT32_MAX,right=UINT32_MAX,ledge=UINT32_MAX;
};
// Owned reader for the observed little-endian VPHY v0100/type0/IVPS format.
// Geometry is in Source model units/axes and the original query-model triangle
// order. It is not a physics tracer, live vcollide object or general PHY loader.
class SourcePhyCompactGeometryCandidate {
public:
    uint32_t checksum=0;
    std::vector<std::vector<SourcePhyConvexGeometryCandidate>>solids;
    // Optional root-first/left-first tree. Relative file references remain raw
    // words; normalized links and leaf ordinals are owned integer indices.
    std::vector<std::vector<SourcePhyCollisionTreeNodeCandidate>>collisionTrees;
    explicit SourcePhyCompactGeometryCandidate(const std::vector<uint8_t>&b,bool retainRawCollision=false){
        auto span=[&](int64_t at,size_t size,int64_t begin,int64_t end){if(at<begin||at>end||uint64_t(size)>uint64_t(end-at))throw std::runtime_error("PHY span outside solid");};
        auto u32=[&](int64_t p){span(p,4,0,int64_t(b.size()));uint32_t v;std::memcpy(&v,b.data()+size_t(p),4);return v;};
        auto i32=[&](int64_t p){const auto v=u32(p);int32_t n;std::memcpy(&n,&v,4);return n;};
        auto u16=[&](int64_t p){span(p,2,0,int64_t(b.size()));uint16_t n;std::memcpy(&n,b.data()+size_t(p),2);return n;};
        auto f32=[&](int64_t p){const auto bits=u32(p);float f;std::memcpy(&f,&bits,4);if(!std::isfinite(f))throw std::runtime_error("PHY nonfinite vertex");return f;};
        span(0,16,0,int64_t(b.size()));const auto header=u32(0),count=u32(8);checksum=u32(12);
        if(header<16||header>b.size()||u32(4)!=0||count==0||count>32767)throw std::runtime_error("unsupported PHY header");int64_t start=header;
        for(uint32_t s=0;s<count;++s){
            const auto size=u32(start);const int64_t end=start+4+int64_t(size);if(size<76)throw std::runtime_error("PHY solid header too small");span(start,80,start,end);span(start,size_t(size)+4,start,int64_t(b.size()));
            if(u32(start+4)!=0x59485056u||u16(start+8)!=0x100||u16(start+10)!=0||u32(start+12)!=size-28||u32(start+28)!=0)throw std::runtime_error("unsupported VPHY solid format");
            const int64_t compact=start+32;span(compact,48,start,end);if(u32(compact+44)!=0x53505649u)throw std::runtime_error("unsupported PHY compact surface");
            const int64_t root=compact+i32(compact+32);std::vector<int64_t>pending{root};std::set<int64_t>visited,ledges;std::vector<SourcePhyConvexGeometryCandidate>convexes;std::vector<SourcePhyCollisionTreeNodeCandidate>tree;std::vector<int64_t>nodeOffsets;
            while(!pending.empty()){
                const int64_t node=pending.back();pending.pop_back();span(node,28,compact+48,end);if(!visited.insert(node).second||visited.size()>1000000)throw std::runtime_error("PHY cyclic/shared or oversized tree");
                if(retainRawCollision){SourcePhyCollisionTreeNodeCandidate entry;std::memcpy(entry.raw.data(),b.data()+size_t(node),28);for(unsigned i=2;i<6;++i)f32(node+int64_t(i)*4);tree.push_back(entry);nodeOffsets.push_back(node);}
                const int32_t right=i32(node),offset=i32(node+4);
                if(right){pending.push_back(node+right);pending.push_back(node+28);continue;}
                if(!offset)throw std::runtime_error("PHY leaf has no ledge");const int64_t ledge=node+offset;span(ledge,16,compact+48,end);if(!ledges.insert(ledge).second)throw std::runtime_error("PHY repeated ledge");
                const uint32_t flags=u32(ledge+8);const uint16_t triangles=u16(ledge+12);const int64_t ledgeEnd=ledge+int64_t(flags>>8)*16;
                if((flags&3)!=0||(flags&0xc)!=4||triangles>32767||triangles==0)throw std::runtime_error("unsupported PHY compact ledge");span(ledge,size_t(16)+size_t(triangles)*16,ledge,ledgeEnd);span(ledge,size_t(ledgeEnd-ledge),compact+48,end);
                SourcePhyConvexGeometryCandidate convex;convex.gameData=u32(ledge+4);const int64_t points=ledge+i32(ledge);
                for(uint16_t t=0;t<triangles;++t){std::array<float,9>v{};const int64_t triangle=ledge+16+int64_t(t)*16;
                    // Original10007080 reads edge indices at +C,+8,+4, then
                    // converts metres/IVP axes to inches/Source axes: X,Z,-Y.
                    for(size_t corner=0;corner<3;++corner){const uint16_t vertex=u16(triangle+12-int64_t(corner)*4);const int64_t p=points+int64_t(vertex)*16;span(p,12,compact+48,end);constexpr float units=39.370079040527344f;const float x=f32(p)*units,y=f32(p+8)*units,z=f32(p+4)*units;v[3*corner]=x;v[3*corner+1]=y;v[3*corner+2]=-z;for(float f:{x,y,z})if(!std::isfinite(f))throw std::runtime_error("PHY converted vertex overflow");}convex.triangles.push_back(v);
                }
                if(retainRawCollision){
                    std::memcpy(convex.rawLedgeHeader.data(),b.data()+size_t(ledge),16);uint32_t last=0;
                    for(uint16_t t=0;t<triangles;++t){std::array<uint32_t,4>raw;std::memcpy(raw.data(),b.data()+size_t(ledge+16+int64_t(t)*16),16);convex.rawTriangles.push_back(raw);for(unsigned corner=1;corner<4;++corner){const uint32_t vertex=raw[corner]&0xffff;if(vertex>last)last=vertex;int32_t opposite=int32_t((raw[corner]>>16)&0x7fff);if(opposite&0x4000)opposite-=0x8000;const int64_t edge=int64_t(t)*4+corner+opposite;if(edge<1||edge%4==0||edge>=int64_t(triangles)*4)throw std::runtime_error("PHY raw opposite edge outside ledge");}}
                    if(last>=32768)throw std::runtime_error("PHY raw vertex index exceeds supported feature range");span(points,(size_t(last)+1)*16,compact+48,end);
                    for(uint32_t i=0;i<=last;++i){std::array<uint32_t,4>raw;std::memcpy(raw.data(),b.data()+size_t(points+int64_t(i)*16),16);convex.rawPoints.push_back(raw);}
                }
                if(retainRawCollision)tree.back().ledge=uint32_t(convexes.size());convexes.push_back(std::move(convex));
            }
            if(retainRawCollision){std::map<int64_t,uint32_t>indices;for(size_t i=0;i<nodeOffsets.size();++i)indices.emplace(nodeOffsets[i],uint32_t(i));for(size_t i=0;i<tree.size();++i)if(tree[i].raw[0]){const int64_t node=nodeOffsets[i];tree[i].left=indices.at(node+28);tree[i].right=indices.at(node+i32(node));}collisionTrees.push_back(std::move(tree));}
            solids.push_back(std::move(convexes));start=end;
        }
        if(start>=int64_t(b.size()))throw std::runtime_error("PHY script missing");bool terminated=false;for(size_t p=size_t(start);p<b.size();++p)if(b[p]==0){terminated=true;break;}if(!terminated)throw std::runtime_error("PHY script not terminated");
    }
};
