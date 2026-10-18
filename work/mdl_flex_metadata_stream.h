#pragma once
#include "mdl_flex_metadata.h"
#include <istream>
#include <map>
#include <array>
#include <algorithm>
// Read only accessed 4KiB pages; all offsets remain bounded by actual file size.
inline std::vector<MdlFlexControllerMetadata> readMdl49FlexMetadataStream(std::istream&stream,uint64_t*bytesRead=nullptr){
 if(bytesRead)*bytesRead=0;stream.clear();stream.seekg(0,std::ios::end);auto end=stream.tellg();if(end<0)throw std::runtime_error("MDL file length unavailable");uint64_t size=uint64_t(end);if(size<408||size>UINT32_MAX)throw std::runtime_error("MDL49 required");
 std::map<uint64_t,std::array<uint8_t,4096>>pages;
 auto byte=[&](uint64_t position)->uint8_t{if(position>=size)throw std::runtime_error("truncated MDL field");uint64_t page=position/4096;auto found=pages.find(page);if(found==pages.end()){std::array<uint8_t,4096>data{};uint64_t offset=page*4096;size_t count=size_t(std::min<uint64_t>(4096,size-offset));stream.clear();stream.seekg(std::streamoff(offset));if(!stream.read(reinterpret_cast<char*>(data.data()),std::streamsize(count)))throw std::runtime_error("MDL page read failure");if(bytesRead)*bytesRead+=count;found=pages.emplace(page,data).first;}return found->second[size_t(position%4096)];};
 auto word=[&](uint64_t p){if(p>size||size-p<4)throw std::runtime_error("truncated MDL field");uint32_t v=0;for(unsigned i=0;i<4;++i)v|=uint32_t(byte(p+i))<<(i*8);return v;};
 if(word(0)!=0x54534449u||word(4)!=49)throw std::runtime_error("MDL49 required");auto length=word(76);if(length!=size)throw std::runtime_error("MDL length mismatch");auto count=word(268),offset=word(272);if(count>length/20||(count&&(offset>length||uint64_t(offset)+uint64_t(count)*20>length)))throw std::runtime_error("invalid flex table");
 auto text=[&](uint64_t record,uint32_t raw){int32_t delta;std::memcpy(&delta,&raw,4);int64_t p=int64_t(record)+delta;if(p<0||uint64_t(p)>=size)throw std::runtime_error("invalid flex string offset");std::string result;for(uint64_t at=uint64_t(p);at<size;++at){auto c=byte(at);if(!c)return result;result.push_back(char(c));}throw std::runtime_error("unterminated flex string");};
 std::vector<MdlFlexControllerMetadata>result;result.reserve(count);for(uint32_t i=0;i<count;++i){uint64_t p=uint64_t(offset)+uint64_t(i)*20;int32_t global;auto g=word(p+8);std::memcpy(&global,&g,4);result.push_back({i,text(p,word(p)),text(p,word(p+4)),global,word(p+12),word(p+16)});}return result;
}
