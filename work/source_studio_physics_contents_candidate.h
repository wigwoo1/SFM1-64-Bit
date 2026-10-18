#pragma once
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>
// Original studio convex-contents callback1016D6C0. gameData0 uses model
// contents; otherwise gameData is ONE-based bone index, stride216,+180.
// Runtime surface-property lookup is deliberately supplied separately.
class SourceStudioPhysicsContentsCandidate {
    uint32_t contents=0;std::vector<uint32_t>bones;
public:
    explicit SourceStudioPhysicsContentsCandidate(const std::vector<uint8_t>&b){
        auto word=[&](size_t at){if(at>b.size()||b.size()-at<4)throw std::runtime_error("studio contents word outside file");uint32_t x;std::memcpy(&x,b.data()+at,4);return x;};
        if(b.size()<400||std::memcmp(b.data(),"IDST",4)||(word(4)!=48&&word(4)!=49))throw std::runtime_error("unsupported studio contents header");const uint32_t length=word(76),count=word(156),offset=word(160);if(length<400||length>b.size()||count>32768||(count&&(offset<400||offset>length||uint64_t(count)*216>length-offset)))throw std::runtime_error("studio contents bone extent");contents=word(332);for(uint32_t i=0;i<count;++i)bones.push_back(word(size_t(offset)+size_t(i)*216+180));
    }
    uint32_t modelContents()const{return contents;}
    size_t boneCount()const{return bones.size();}
    uint32_t forConvex(uint32_t gameData)const{if(!gameData)return contents;if(gameData>bones.size())throw std::runtime_error("studio convex game data outside bone contents");return bones[gameData-1];}
};
