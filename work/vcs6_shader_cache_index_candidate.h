#pragma once
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

// Owned index metadata only. Compressed bytes require a separate decoder.
class Vcs6ShaderCacheIndexCandidate {
    using Pair = std::pair<uint32_t,uint32_t>;
    std::vector<Pair> entries_, aliases_;
    uint32_t total_=0, dynamic_=0;
    static auto find(const std::vector<Pair>& table,uint32_t key) {
        return std::lower_bound(table.begin(),table.end(),key,
            [](const Pair& pair,uint32_t value){return pair.first<value;});
    }
public:
    struct Selection { uint32_t normalized, resolved, begin, end; };
    explicit Vcs6ShaderCacheIndexCandidate(const std::vector<uint8_t>& data) {
        auto fail=[](){throw std::runtime_error("invalid VCS6 index");};
        auto word=[&](size_t at)->uint32_t {
            if(at>data.size()||data.size()-at<4)fail();
            return uint32_t(data[at])|(uint32_t(data[at+1])<<8)|
                (uint32_t(data[at+2])<<16)|(uint32_t(data[at+3])<<24);
        };
        if(data.size()<40||data.size()>256u*1024u*1024u||word(0)!=6)fail();
        total_=word(4);dynamic_=word(8);auto count=word(20);
        if(!total_||!dynamic_||total_%dynamic_||count<2||count>(data.size()-28)/8)fail();
        uint32_t staticCount=total_/dynamic_;
        for(uint32_t i=0;i<count;++i)entries_.emplace_back(word(28+size_t(i)*8),word(32+size_t(i)*8));
        if(entries_.back()!=Pair(UINT32_MAX,static_cast<uint32_t>(data.size())))fail();
        for(size_t i=0;i+1<entries_.size();++i) {
            if(entries_[i].first>=staticCount||entries_[i].second>=entries_[i+1].second||
                (i&&entries_[i-1].first>=entries_[i].first))fail();
        }
        size_t at=28+size_t(count)*8;auto aliasCount=word(at);at+=4;
        if(aliasCount>(data.size()-at)/8)fail();
        for(uint32_t i=0;i<aliasCount;++i) {
            Pair pair(word(at),word(at+4));at+=8;
            auto original=find(entries_,pair.first),target=find(entries_,pair.second);
            if(pair.first>=staticCount||(original!=entries_.end()&&original->first==pair.first)||
                target==entries_.end()||target->first!=pair.second||pair.second==UINT32_MAX||
                (!aliases_.empty()&&aliases_.back().first>=pair.first))fail();
            aliases_.push_back(pair);
        }
        if(entries_.front().second!=at)fail();
    }
    Selection select(uint32_t argument,uint32_t dynamicIndex) const {
        if(argument>=total_||dynamicIndex>=dynamic_)throw std::runtime_error("VCS6 combo range");
        uint32_t normalized=argument/dynamic_,resolved=normalized;
        auto alias=find(aliases_,normalized);
        if(alias!=aliases_.end()&&alias->first==normalized)resolved=alias->second;
        auto entry=find(entries_,resolved);
        if(entry==entries_.end()||entry->first!=resolved||entry->first==UINT32_MAX)
            throw std::runtime_error("VCS6 static combo absent");
        return {normalized,resolved,entry->second,(entry+1)->second};
    }
};
