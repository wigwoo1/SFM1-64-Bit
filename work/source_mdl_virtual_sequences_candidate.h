#pragma once
#include "source_mdl_attachment_capacity_candidate.h"
#include <array>
#include "source_sequence_name_cache_candidate.h"
struct SourceMdlLocalSequenceCandidate {std::string label;uint32_t flags=0,activity=0;std::array<float,6>bounds{};};
struct SourceMdlSequenceMetadataCandidate {std::vector<SourceMdlLocalSequenceCandidate>sequences;std::vector<std::string>includes;};
inline SourceMdlSequenceMetadataCandidate readSourceMdlSequenceMetadataCandidate(const std::vector<uint8_t>&b){
    auto span=[&](uint64_t at,uint64_t n){if(at>b.size()||n>b.size()-at)throw std::runtime_error("MDL sequence span");};
    auto integer=[&](size_t at){span(at,4);int32_t v;std::memcpy(&v,b.data()+at,4);return v;};
    auto text=[&](int64_t at){if(at<0)throw std::runtime_error("MDL sequence string offset");span(uint64_t(at),1);size_t end=size_t(at);while(end<b.size()&&b[end])++end;if(end==b.size()||end-size_t(at)>1024)throw std::runtime_error("MDL sequence string span");return std::string(reinterpret_cast<const char*>(b.data()+at),end-size_t(at));};
    if(b.size()<0x158||integer(0)!=0x54534449||integer(4)!=49||integer(76)<0||size_t(integer(76))!=b.size())throw std::runtime_error("MDL49 sequence identity");
    const auto count=integer(0xbc),start=integer(0xc0),includes=integer(0x150),offset=integer(0x154);
    if(count<0||count>4096||start<0||includes<0||includes>64||offset<0||(count&&start<0x158)||(includes&&offset<0x158))throw std::runtime_error("MDL sequence limits");
    span(uint32_t(start),uint64_t(count)*212);span(uint32_t(offset),uint64_t(includes)*8);
    SourceMdlSequenceMetadataCandidate out;
    for(int32_t i=0;i<count;++i){const size_t at=size_t(start)+size_t(i)*212;SourceMdlLocalSequenceCandidate s;s.label=text(int64_t(at)+integer(at+4));
        if(s.label.empty())throw std::runtime_error("empty sequence label");for(unsigned char c:s.label)if(c<32||c>126)throw std::runtime_error("non-ASCII sequence label unsupported");
        std::memcpy(&s.flags,b.data()+at+12,4);std::memcpy(&s.activity,b.data()+at+16,4);std::memcpy(s.bounds.data(),b.data()+at+32,24);out.sequences.push_back(std::move(s));
    }
    for(int32_t i=0;i<includes;++i){const size_t at=size_t(offset)+size_t(i)*8;out.includes.push_back(sourceMdlIncludeIdentityCandidate(text(int64_t(at)+integer(at+4))));}
    return out;
}
struct SourceVirtualSequenceEntryCandidate {uint32_t flags=0,activity=0,group=0,index=0;};
static_assert(sizeof(SourceVirtualSequenceEntryCandidate)==16,"semantic four-word sequence entry");
struct SourceVirtualSequencesCandidate {std::vector<SourceVirtualSequenceEntryCandidate>entries;std::vector<std::vector<uint32_t>>localToMaster;};
inline std::string sourceSequenceLabelIdentityCandidate(std::string label){
    if(label.empty()||label.size()>1024)throw std::runtime_error("sequence label length");
    for(char&c:label){if(uint8_t(c)<32||uint8_t(c)>126)throw std::runtime_error("sequence label encoding");if(c>='A'&&c<='Z')c=char(c-'A'+'a');}return label;
}
enum class SourceSequenceNameCacheModeCandidate {Disabled,Enabled};
// Native10013060 append policy with explicit optional name-cache mode.
// Caller supplies groups in actual append order. This is not an include loader,
// bone/animation map or native cache. Lookup scans ONLY the pre-append entries,
// so equal labels introduced within a single group can have separate entries.
inline SourceVirtualSequencesCandidate sourceVirtualSequencesCandidate(const std::vector<SourceMdlSequenceMetadataCandidate>&groups,SourceSequenceNameCacheModeCandidate mode=SourceSequenceNameCacheModeCandidate::Disabled){
    if(groups.empty()||groups.size()>64)throw std::runtime_error("sequence group limit");
    SourceVirtualSequencesCandidate out;out.localToMaster.resize(groups.size());SourceSequenceNameCacheCandidate cache;
    for(uint32_t group=0;group<groups.size();++group){const auto&src=groups[group].sequences;if(src.size()>4096)throw std::runtime_error("local sequence limit");const size_t old=out.entries.size();
        for(uint32_t index=0;index<src.size();++index){const auto label=sourceSequenceLabelIdentityCandidate(src[index].label);size_t master=0;
            if(mode==SourceSequenceNameCacheModeCandidate::Enabled)master=cache.find(label).value_or(uint32_t(old));
            else for(;master<old;++master){const auto&e=out.entries[master];if(sourceSequenceLabelIdentityCandidate(groups.at(e.group).sequences.at(e.index).label)==label)break;}
            const SourceVirtualSequenceEntryCandidate incoming{src[index].flags,src[index].activity,group,index};
            if(master==old){if(out.entries.size()>=4096)throw std::runtime_error("virtual sequence limit");master=out.entries.size();out.entries.push_back(incoming);}
            else {const auto&e=out.entries[master];if(groups.at(e.group).sequences.at(e.index).flags&0x800)out.entries[master]=incoming;}
            out.localToMaster[group].push_back(uint32_t(master));
        }
        if(mode==SourceSequenceNameCacheModeCandidate::Enabled)for(size_t master=old;master<out.entries.size();++master){const auto&e=out.entries[master];cache.insert(sourceSequenceLabelIdentityCandidate(groups.at(e.group).sequences.at(e.index).label),uint32_t(master));}
    }return out;
}
// Original105DA450: invalid signed index selects virtual index0. The caller
// must provide a nonempty resolved table, not clamp into root-local sequences.
inline SourceVirtualSequenceEntryCandidate sourceVirtualSequenceCandidate(const SourceVirtualSequencesCandidate&table,int32_t index){
    if(table.entries.empty())throw std::runtime_error("empty virtual sequence table");return table.entries[index<0||uint64_t(index)>=table.entries.size()?0:size_t(index)];
}
