#pragma once
#include "vcs6_shader_cache_index_candidate.h"
#include "dependencies/lzma-decoder/LzmaDec.h"
#include <cstdlib>
#include <map>
#include <memory>
#include <mutex>

using Vcs6DynamicProgramsCandidate=std::map<uint32_t,std::vector<uint8_t>>;
inline Vcs6DynamicProgramsCandidate decodeVcs6StaticBlockCandidate(
    const std::vector<uint8_t>& data,Vcs6ShaderCacheIndexCandidate::Selection range) {
    auto word=[&](size_t at)->uint32_t {
        if(at>data.size()||data.size()-at<4)throw std::runtime_error("VCS6 word bounds");
        return uint32_t(data[at])|(uint32_t(data[at+1])<<8)|
            (uint32_t(data[at+2])<<16)|(uint32_t(data[at+3])<<24);
    };
    const ISzAlloc allocator={
        [](ISzAllocPtr,size_t size)->void*{return std::malloc(size);},
        [](ISzAllocPtr,void* address){std::free(address);}};
    Vcs6DynamicProgramsCandidate records;
    size_t at=range.begin,totalDecoded=0;bool ended=false;
    while(at<range.end) {
        if(range.end-at<4)throw std::runtime_error("VCS6 token bounds");
        auto token=word(at);at+=4;
        if(token==UINT32_MAX){if(at!=range.end)throw std::runtime_error("VCS6 trailing block bytes");ended=true;break;}
        auto size=token&0x3fffffffu;
        if((token>>30)!=1||size<17||size>range.end-at)throw std::runtime_error("VCS6 block encoding");
        if(word(at)!=0x414d5a4c)throw std::runtime_error("VCS6 LZMA signature");
        auto decodedSize=word(at+4),compressedSize=word(at+8);
        totalDecoded+=decodedSize;
        if(decodedSize<8||decodedSize>64u*1024u*1024u||totalDecoded>256u*1024u*1024u||
            compressedSize!=size-17||data[at+12]>=225||word(at+13)>64u*1024u*1024u)
            throw std::runtime_error("VCS6 LZMA size/properties");
        std::vector<uint8_t> decoded(decodedSize);SizeT written=decoded.size(),consumed=compressedSize;
        ELzmaStatus status=LZMA_STATUS_NOT_SPECIFIED;
        // Source blocks can omit EOS: declared decoded size is the stopping point.
        auto result=LzmaDecode(decoded.data(),&written,data.data()+at+17,&consumed,
            data.data()+at+12,5,LZMA_FINISH_ANY,&status,&allocator);
        if(result!=SZ_OK||written!=decodedSize)throw std::runtime_error("VCS6 LZMA decode");
        size_t pos=0;
        auto decodedWord=[&](size_t offset){return uint32_t(decoded[offset])|
            (uint32_t(decoded[offset+1])<<8)|(uint32_t(decoded[offset+2])<<16)|(uint32_t(decoded[offset+3])<<24);};
        while(pos<decoded.size()) {
            if(decoded.size()-pos<8)throw std::runtime_error("VCS6 dynamic header");
            auto key=decodedWord(pos),length=decodedWord(pos+4);pos+=8;
            if(key>=word(8)||records.count(key)||length<8||length%4||length>decoded.size()-pos)
                throw std::runtime_error("VCS6 dynamic record");
            auto version=decodedWord(pos)>>16;
            if((version!=0xfffe&&version!=0xffff)||decodedWord(pos+length-4)!=0x0000ffff)
                throw std::runtime_error("VCS6 D3D bytecode envelope");
            records.emplace(key,std::vector<uint8_t>(decoded.begin()+pos,decoded.begin()+pos+length));pos+=length;
        }
        at+=size;
    }
    if(!ended)throw std::runtime_error("VCS6 missing sentinel");
    return records;
}

inline std::vector<uint8_t> vcs6ShaderBytecodeCandidate(
    const std::vector<uint8_t>& data,uint32_t argument,uint32_t dynamicIndex) {
    Vcs6ShaderCacheIndexCandidate index(data);
    auto records=decodeVcs6StaticBlockCandidate(data,index.select(argument,dynamicIndex));
    auto record=records.find(dynamicIndex);
    if(record==records.end())throw std::runtime_error("VCS6 dynamic combo absent");
    return record->second;
}

// Immutable source ownership and shared program lifetimes. Metadata/map overhead
// is additional to the explicit retained-bytecode budget. Cold decodes serialize.
class RetainedVcs6ShaderCacheCandidate {
    const std::vector<uint8_t> data_;
    const Vcs6ShaderCacheIndexCandidate index_;
    const size_t budget_;
    std::mutex mutex_;
    std::map<uint32_t,std::shared_ptr<const Vcs6DynamicProgramsCandidate>> blocks_;
    size_t retainedBytes_=0,decodedBlocks_=0;
public:
    explicit RetainedVcs6ShaderCacheCandidate(std::vector<uint8_t> data,
        size_t budget=256u*1024u*1024u):data_(std::move(data)),index_(data_),budget_(budget){}
    std::shared_ptr<const std::vector<uint8_t>> shader(uint32_t argument,uint32_t dynamicIndex) {
        auto range=index_.select(argument,dynamicIndex);
        std::lock_guard<std::mutex> lock(mutex_);
        auto block=blocks_.find(range.resolved);
        if(block==blocks_.end()) {
            auto programs=std::make_shared<const Vcs6DynamicProgramsCandidate>(decodeVcs6StaticBlockCandidate(data_,range));
            size_t bytes=0;for(const auto& pair:*programs)bytes+=pair.second.size();
            if(bytes>budget_-retainedBytes_)throw std::runtime_error("VCS6 retained bytecode budget");
            block=blocks_.emplace(range.resolved,std::move(programs)).first;
            retainedBytes_+=bytes;++decodedBlocks_;
        }
        auto program=block->second->find(dynamicIndex);
        if(program==block->second->end())throw std::runtime_error("VCS6 dynamic combo absent");
        return std::shared_ptr<const std::vector<uint8_t>>(block->second,&program->second);
    }
    size_t decodedBlocks(){std::lock_guard<std::mutex> lock(mutex_);return decodedBlocks_;}
    size_t retainedBytes(){std::lock_guard<std::mutex> lock(mutex_);return retainedBytes_;}
};
