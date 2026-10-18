#pragma once
#include "queued_vertex_metadata_candidate.h"
// Owned finite-batch form of native 4096-word blocks: word0 publication state2,
// command payload, then terminator0. Does not reproduce the native 500-slot
// producer/consumer ring, blocking waits, pool reuse or atomic publication.
inline std::vector<std::vector<uint32_t>> queuedMetadataBlocksCandidate(
    const std::vector<VertexMetadataUploadCandidate>& commands){
    std::vector<std::vector<uint32_t>> blocks;std::vector<uint32_t> current{2};
    for(const auto& command:commands){
        auto encoded=queuedVertexMetadataCandidate(command);
        if(encoded.size()>4094)throw std::invalid_argument("metadata block command capacity");
        if(current.size()+encoded.size()+1>4096){current.push_back(0);blocks.push_back(std::move(current));current={2};}
        current.insert(current.end(),encoded.begin(),encoded.end());
    }
    if(current.size()>1){current.push_back(0);blocks.push_back(std::move(current));}
    return blocks;
}
inline std::vector<VertexMetadataUploadCandidate> decodeQueuedMetadataBlocksCandidate(
    const std::vector<std::vector<uint32_t>>& blocks,uint32_t integerLimit,uint32_t booleanLimit){
    if(blocks.size()>500)throw std::invalid_argument("metadata block batch capacity");
    std::vector<VertexMetadataUploadCandidate> result;
    for(const auto& block:blocks){
        if(block.size()<2||block.size()>4096||block.front()!=2||block.back()!=0)throw std::invalid_argument("metadata block envelope");
        auto commands=decodeQueuedVertexMetadataCandidate({block.begin()+1,block.end()-1},integerLimit,booleanLimit);
        result.insert(result.end(),commands.begin(),commands.end());
    }
    return result;
}
template<class Upload> inline size_t dispatchQueuedMetadataBlocksCandidate(
    const std::vector<std::vector<uint32_t>>& blocks,uint32_t integerLimit,uint32_t booleanLimit,Upload upload){
    const auto commands=decodeQueuedMetadataBlocksCandidate(blocks,integerLimit,booleanLimit);
    for(const auto& command:commands)upload(command);
    return commands.size();
}
