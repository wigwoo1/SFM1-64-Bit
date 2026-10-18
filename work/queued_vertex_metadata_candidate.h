#pragma once
#include "generic_lighting_metadata_candidate.h"
// Device queue opcodes, distinct from the material dynamic-selector stream.
// 11 = vertex BOOL constants; 12 = vertex INT4 constants.
inline std::vector<uint32_t> queuedVertexMetadataCandidate(const VertexMetadataUploadCandidate& command){
    if(command.kind!=VertexMetadataKindCandidate::Integer&&command.kind!=VertexMetadataKindCandidate::Boolean)throw std::invalid_argument("metadata queue kind");
    const size_t width=command.kind==VertexMetadataKindCandidate::Integer?4:1;
    if(command.words.size()%width||command.words.size()/width>4096)throw std::invalid_argument("metadata queue dimensions");
    std::vector<uint32_t> result{width==4?12u:11u,command.index,static_cast<uint32_t>(command.words.size()/width)};
    result.insert(result.end(),command.words.begin(),command.words.end());return result;
}
inline std::vector<VertexMetadataUploadCandidate> decodeQueuedVertexMetadataCandidate(
    const std::vector<uint32_t>& stream,uint32_t integerLimit,uint32_t booleanLimit){
    if(stream.size()>65536)throw std::invalid_argument("metadata queue size");
    std::vector<VertexMetadataUploadCandidate> result;size_t at=0;
    while(at<stream.size()){
        if(stream.size()-at<3)throw std::invalid_argument("metadata queue header");
        const uint32_t opcode=stream[at++],index=stream[at++],count=stream[at++];
        if(opcode!=11&&opcode!=12)throw std::invalid_argument("metadata queue opcode");
        const size_t width=opcode==12?4:1;const uint32_t limit=width==4?integerLimit:booleanLimit;
        if(index>limit||count>limit-index||count>(stream.size()-at)/width)throw std::invalid_argument("metadata queue range");
        const size_t words=size_t(count)*width;
        result.push_back({width==4?VertexMetadataKindCandidate::Integer:VertexMetadataKindCandidate::Boolean,index,{stream.begin()+at,stream.begin()+at+words},false});at+=words;
    }
    return result;
}
