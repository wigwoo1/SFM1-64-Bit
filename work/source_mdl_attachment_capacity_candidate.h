#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
struct SourceMdlAttachmentCapacityMetadataCandidate {uint32_t localAttachments=0;std::vector<std::string>includes;};
inline std::string sourceMdlIncludeIdentityCandidate(std::string name){
    if(name.empty())throw std::runtime_error("empty included model");
    for(char&c:name){if(c=='\\')c='/';if(c>='A'&&c<='Z')c=char(c-'A'+'a');if(uint8_t(c)<32||uint8_t(c)>126||c==':')throw std::runtime_error("unsupported included model path");}
    if(name.front()=='/'||name.find("//")!=std::string::npos)throw std::runtime_error("absolute/empty included model segment");
    size_t at=0;while(at<name.size()){const auto end=name.find('/',at);const auto part=name.substr(at,end-at);if(part==".."||part==".")throw std::runtime_error("included model traversal");if(end==std::string::npos)break;at=end+1;}
    if(name.size()<4||name.substr(name.size()-4)!=".mdl")throw std::runtime_error("included model extension");return name;
}
inline SourceMdlAttachmentCapacityMetadataCandidate readSourceMdlAttachmentCapacityMetadataCandidate(const std::vector<uint8_t>&b){
    auto span=[&](uint64_t o,uint64_t n){if(o>b.size()||n>b.size()-o)throw std::runtime_error("MDL attachment/include span");};
    auto word=[&](size_t o){span(o,4);int32_t v;std::memcpy(&v,b.data()+o,4);return v;};
    if(b.size()<0x158||word(0)!=0x54534449||word(4)!=49||word(76)<0||size_t(word(76))!=b.size())throw std::runtime_error("MDL49 attachment capacity identity");
    const auto count=word(0xf0),offset=word(0xf4),includes=word(0x150),start=word(0x154);
    if(count<0||count>4096||offset<0||includes<0||includes>64||start<0)throw std::runtime_error("MDL attachment/include limits");
    if((count&&offset<0x158)||(includes&&start<0x158))throw std::runtime_error("MDL attachment/include header overlap");
    span(uint32_t(offset),uint64_t(count)*92);span(uint32_t(start),uint64_t(includes)*8);
    SourceMdlAttachmentCapacityMetadataCandidate out;out.localAttachments=uint32_t(count);
    for(int32_t i=0;i<includes;++i){const size_t record=size_t(start)+size_t(i)*8;const int64_t name=int64_t(record)+word(record+4);if(name<0)throw std::runtime_error("included model string offset");span(uint64_t(name),1);size_t end=size_t(name);while(end<b.size()&&b[end])++end;if(end==b.size()||end-size_t(name)>1024)throw std::runtime_error("included model string bounds");out.includes.push_back(sourceMdlIncludeIdentityCandidate(std::string(reinterpret_cast<const char*>(b.data()+name),end-size_t(name))));}
    return out;
}
struct SourceMdlAttachmentCapacityCandidate {uint32_t upperBound=0,expandedGroups=0;std::vector<std::string>assets;};
// Conservative count proof, NOT a virtual attachment table/ordering/bone map.
// Native10012380 only keeps old entries or adds source local attachments;
// deduplication and unmapped-bone filtering can lower this count. All include
// occurrences count, even repeated assets. Missing/ambiguous/cyclic sources
// must throw; never infer a valid attachment from index<=this upper bound.
template<class LoadMetadata>
SourceMdlAttachmentCapacityCandidate sourceMdlAttachmentCapacityCandidate(const std::string&root,LoadMetadata load){
    SourceMdlAttachmentCapacityCandidate out;std::vector<std::string>active;
    auto visit=[&](auto&&self,const std::string&raw)->void{
        const auto name=sourceMdlIncludeIdentityCandidate(raw);
        if(active.size()>=64||out.expandedGroups>=256||std::find(active.begin(),active.end(),name)!=active.end())throw std::runtime_error("MDL include cycle/depth/group limit");
        active.push_back(name);const auto metadata=load(name);
        if(metadata.localAttachments>4096||metadata.includes.size()>64||metadata.localAttachments>uint32_t(INT32_MAX)-out.upperBound)throw std::runtime_error("MDL attachment capacity overflow/limit");
        out.upperBound+=metadata.localAttachments;++out.expandedGroups;out.assets.push_back(name);
        for(const auto&child:metadata.includes)self(self,child);active.pop_back();
    };visit(visit,root);return out;
}
