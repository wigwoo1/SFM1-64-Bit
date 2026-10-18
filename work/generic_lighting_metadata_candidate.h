#pragma once
#include "generic_lighting_constants_record_candidate.h"
#include <vector>
enum class VertexMetadataKindCandidate {Integer,Boolean};
struct VertexMetadataUploadCandidate {
    VertexMetadataKindCandidate kind;
    uint32_t index;
    std::vector<uint32_t> words;
    bool force=false;
};
inline std::vector<VertexMetadataUploadCandidate> genericLightingMetadataCandidate(
    const GenericLightingConstantsRecordCandidate& record){
    return {{VertexMetadataKindCandidate::Integer,0,{record.words.begin()+129,record.words.begin()+133},false},
            {VertexMetadataKindCandidate::Boolean,0,{record.words.begin()+133,record.words.end()},false}};
}
// Vertex-only metadata path. Entire changed ranges upload, with per-callback
// commit. Raw boolean words are preserved, not normalized by this cache.
class RetainedVertexMetadataCandidate {
    std::vector<uint32_t> integers_,booleans_;
    std::vector<uint8_t> integerKnown_,booleanKnown_;
public:
    RetainedVertexMetadataCandidate(uint32_t integerLimit,uint32_t booleanLimit){
        if(integerLimit>4096||booleanLimit>4096)throw std::invalid_argument("metadata capacity");
        integers_.resize(size_t(integerLimit)*4);booleans_.resize(booleanLimit);
        integerKnown_.resize(integerLimit);booleanKnown_.resize(booleanLimit);
    }
    void invalidate(){std::fill(integerKnown_.begin(),integerKnown_.end(),uint8_t(0));std::fill(booleanKnown_.begin(),booleanKnown_.end(),uint8_t(0));}
    template<class Upload> size_t apply(const std::vector<VertexMetadataUploadCandidate>& plan,Upload upload){
        for(const auto& command:plan){
            if(command.kind!=VertexMetadataKindCandidate::Integer&&command.kind!=VertexMetadataKindCandidate::Boolean)throw std::invalid_argument("metadata kind");
            const size_t width=command.kind==VertexMetadataKindCandidate::Integer?4:1;
            const auto& known=width==4?integerKnown_:booleanKnown_;
            if(command.words.size()%width||command.index>known.size()||command.words.size()/width>known.size()-command.index)throw std::invalid_argument("metadata range");
        }
        size_t sent=0;
        for(const auto& command:plan){
            const size_t width=command.kind==VertexMetadataKindCandidate::Integer?4:1;
            auto& known=width==4?integerKnown_:booleanKnown_;auto& words=width==4?integers_:booleans_;
            const size_t count=command.words.size()/width;
            bool changed=command.force;
            for(size_t n=0;n<count;++n)changed=changed||!known[command.index+n];
            changed=changed||!std::equal(command.words.begin(),command.words.end(),words.begin()+size_t(command.index)*width);
            if(!changed)continue;
            upload(command);
            std::copy(command.words.begin(),command.words.end(),words.begin()+size_t(command.index)*width);
            std::fill(known.begin()+command.index,known.begin()+command.index+count,uint8_t(1));++sent;
        }
        return sent;
    }
};
