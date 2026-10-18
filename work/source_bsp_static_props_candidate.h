#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

// Checked, owned VBSP20 / uncompressed sprp5 records only. Original loader
// 101C7160 selects 60 bytes at 101C7232, then initializes in disk order.
// This order identifies props; it is NOT spatial partition enumeration order.
struct SourceBspStaticPropRecordCandidate {
    // lightingOrigin is raw preserved data unless propFlags bit2 activates it.
    // Real benchmark records contain inactive NaN patterns; do not evaluate.
    std::array<float,3>origin{},angles{},lightingOrigin{};
    uint16_t model=0,firstLeaf=0,leafCount=0;
    uint8_t solidType=0,propFlags=0;
    int32_t skin=0;
    float fadeMin=0,fadeMax=0,forcedFadeScale=0;
};
class SourceBspStaticPropsCandidate {
    std::vector<std::string>names_;
    std::vector<uint16_t>leaves_;
    std::vector<SourceBspStaticPropRecordCandidate>records_;
public:
    explicit SourceBspStaticPropsCandidate(const std::vector<uint8_t>&b){
        auto span=[&](size_t at,size_t count){if(at>b.size()||count>b.size()-at)throw std::runtime_error("static props file span");};
        auto word=[&](size_t at){span(at,4);uint32_t v;std::memcpy(&v,b.data()+at,4);return v;};
        auto half=[&](size_t at){span(at,2);uint16_t v;std::memcpy(&v,b.data()+at,2);return v;};
        auto number=[&](size_t at){span(at,4);float v;std::memcpy(&v,b.data()+at,4);if(!std::isfinite(v))throw std::runtime_error("nonfinite static prop field");return v;};
        span(0,1036);if(std::memcmp(b.data(),"VBSP",4)||word(4)!=20)throw std::runtime_error("unsupported static props BSP");
        auto lump=[&](size_t index){const size_t p=8+index*16;size_t at=word(p),size=word(p+4);span(at,size);if(word(p+12))throw std::runtime_error("compressed static props dependency");if(size&&at<1036)throw std::runtime_error("static props dependency overlaps header");return std::array<size_t,3>{at,size,word(p+8)};};
        const auto worldLeaves=lump(10),game=lump(35);
        if(worldLeaves[2]!=1||worldLeaves[1]%32||game[2]||game[1]<4)throw std::runtime_error("unsupported static props dependencies");
        const size_t worldLeafCount=worldLeaves[1]/32,entries=word(game[0]);
        if(entries>(game[1]-4)/16)throw std::runtime_error("static props game directory extent");
        size_t offset=0,length=0;bool found=false;
        for(size_t i=0;i<entries;++i){const size_t p=game[0]+4+i*16;if(word(p)!=0x73707270u)continue;if(found)throw std::runtime_error("duplicate static prop lump");found=true;if(half(p+4)||half(p+6)!=5)throw std::runtime_error("unsupported static prop lump encoding");offset=word(p+8);length=word(p+12);span(offset,length);if(offset<game[0]+4+entries*16||offset>game[0]+game[1]||length>game[0]+game[1]-offset)throw std::runtime_error("static prop lump outside game payload");}
        if(!found)throw std::runtime_error("static prop lump unavailable");
        size_t p=offset;const size_t end=offset+length;
        auto count=[&](size_t stride,size_t limit){if(p>end||end-p<4)throw std::runtime_error("static prop count truncated");const size_t n=word(p);p+=4;if(n>limit||n>(end-p)/stride)throw std::runtime_error("static prop table extent");return n;};
        const size_t models=count(128,65536);names_.reserve(models);
        for(size_t i=0;i<models;++i,p+=128){size_t n=0;while(n<128&&b[p+n])++n;if(!n||n==128)throw std::runtime_error("static prop model name extent");std::string name(reinterpret_cast<const char*>(b.data()+p),n);std::string part;for(unsigned char c:name){if(c<32||c>126||c==':'||c=='|'||c=='*'||c=='?'||c=='"'||c=='<'||c=='>')throw std::runtime_error("unsafe static prop model name");if(c=='/'||c=='\\'){if(part.empty()||part=="."||part=="..")throw std::runtime_error("unsafe static prop model segment");part.clear();}else part+=char(c);}if(part.empty()||part=="."||part==".."||name.size()<11||name.compare(0,7,"models/")||name.compare(name.size()-4,4,".mdl"))throw std::runtime_error("unsupported static prop model path");names_.push_back(std::move(name));}
        const size_t leaves=count(2,1u<<24);leaves_.reserve(leaves);for(size_t i=0;i<leaves;++i,p+=2){const auto leaf=half(p);if(leaf>=worldLeafCount)throw std::runtime_error("static prop world leaf index");leaves_.push_back(leaf);}
        const size_t props=count(60,65536);if(props*60!=end-p)throw std::runtime_error("static prop record payload extent");records_.reserve(props);
        for(size_t i=0;i<props;++i,p+=60){SourceBspStaticPropRecordCandidate r;r.solidType=b[p+30];r.propFlags=b[p+31];for(size_t j=0;j<3;++j){r.origin[j]=number(p+j*4);r.angles[j]=number(p+12+j*4);std::memcpy(&r.lightingOrigin[j],b.data()+p+44+j*4,4);if((r.propFlags&2)&&!std::isfinite(r.lightingOrigin[j]))throw std::runtime_error("nonfinite active prop lighting origin");}r.model=half(p+24);r.firstLeaf=half(p+26);r.leafCount=half(p+28);std::memcpy(&r.skin,b.data()+p+32,4);r.fadeMin=number(p+36);r.fadeMax=number(p+40);r.forcedFadeScale=number(p+56);if(r.model>=names_.size()||r.firstLeaf>leaves_.size()||r.leafCount>leaves_.size()-r.firstLeaf)throw std::runtime_error("static prop record reference");if(r.solidType!=0&&r.solidType!=2&&r.solidType!=6)throw std::runtime_error("unsupported static prop solid type");records_.push_back(r);}
    }
    const auto&models()const{return names_;}
    const auto&leaves()const{return leaves_;}
    const auto&records()const{return records_;}
};

// Collision-facing part of original101C4CF0 + actual ICollideable getters.
// Model identity is a caller-owned token, never a truncated native pointer.
// No asset lookup, cache absence inference, broadphase ordering or PHY binding.
struct SourceStaticPropCollisionBindingCandidate {
    uint32_t referenceHandle=0,solidType=0,solidFlags=0;
    uint64_t modelId=0;
    std::array<float,3>origin{},angles{};
};
inline SourceStaticPropCollisionBindingCandidate sourceStaticPropCollisionBindingCandidate(
    const SourceBspStaticPropRecordCandidate&r,size_t index,uint64_t modelId){
    if(index>65535||!modelId||(r.solidType!=0&&r.solidType!=2&&r.solidType!=6))throw std::runtime_error("static prop binding identity/type");
    for(const auto&v:{r.origin,r.angles})for(float x:v)if(!std::isfinite(x))throw std::runtime_error("nonfinite static prop pose");
    SourceStaticPropCollisionBindingCandidate out;out.referenceHandle=0x40000000u|uint32_t(index);out.solidType=r.solidType;out.modelId=modelId;out.origin=r.origin;if(r.solidType==6)out.angles=r.angles;
    // Original slot12 (101C4BB0) returns zero, independently of disk propFlags.
    return out;
}
