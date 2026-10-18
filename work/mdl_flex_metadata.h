#pragma once
#include <vector>
#include <string>
#include <cstring>
#include <stdexcept>
#include <cstdint>
struct MdlFlexControllerMetadata {uint32_t localIndex;std::string type,name;int32_t diskGlobalIndex;uint32_t minBits,maxBits;};
inline std::vector<MdlFlexControllerMetadata> readMdl49FlexMetadata(const std::vector<uint8_t>& b){
 auto word=[&](size_t p){if(p>b.size()||b.size()-p<4)throw std::runtime_error("truncated MDL field");uint32_t v;std::memcpy(&v,b.data()+p,4);return v;};
 if(b.size()<408||word(0)!=0x54534449u||word(4)!=49)throw std::runtime_error("MDL49 required");
 auto length=word(76);if(length!=b.size())throw std::runtime_error("MDL length mismatch");
 auto count=word(268),offset=word(272);if(count>length/20|| (count&&(offset>length||uint64_t(offset)+uint64_t(count)*20>length)))throw std::runtime_error("invalid flex table");
 auto text=[&](size_t record,uint32_t raw){int32_t delta;std::memcpy(&delta,&raw,4);int64_t p=int64_t(record)+delta;if(p<0||uint64_t(p)>=length)throw std::runtime_error("invalid flex string offset");size_t end=size_t(p);while(end<length&&b[end])++end;if(end==length)throw std::runtime_error("unterminated flex string");return std::string(reinterpret_cast<const char*>(b.data()+size_t(p)),end-size_t(p));};
 std::vector<MdlFlexControllerMetadata> result;result.reserve(count);
 for(uint32_t i=0;i<count;++i){size_t p=size_t(offset)+size_t(i)*20;int32_t global;auto g=word(p+8);std::memcpy(&global,&g,4);result.push_back({i,text(p,word(p)),text(p,word(p+4)),global,word(p+12),word(p+16)});}return result;
}
