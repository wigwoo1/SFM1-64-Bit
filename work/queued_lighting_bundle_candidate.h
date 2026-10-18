#pragma once
#include "generic_lighting_upload_bundle_candidate.h"
#include "queued_vertex_metadata_candidate.h"
// Vertex device-queue subset:10 FLOAT4,11 BOOL,12 INT4. Not material commands.
inline std::vector<uint32_t> queuedLightingBundleCandidate(const GenericLightingUploadBundleCandidate& bundle){
    std::vector<uint32_t> result;
    for(const auto& command:bundle.floats.uploads){
        if(command.pixel||command.words.size()%4||command.words.size()/4>4096)throw std::invalid_argument("lighting queue float dimensions");
        result.insert(result.end(),{10,command.index,static_cast<uint32_t>(command.words.size()/4)});
        result.insert(result.end(),command.words.begin(),command.words.end());
    }
    for(const auto& command:bundle.metadata){auto encoded=queuedVertexMetadataCandidate(command);result.insert(result.end(),encoded.begin(),encoded.end());}
    if(result.size()>65536)throw std::invalid_argument("lighting queue size");
    return result;
}
inline GenericLightingUploadBundleCandidate decodeQueuedLightingBundleCandidate(
    const std::vector<uint32_t>& stream,uint32_t floatLimit,uint32_t integerLimit,uint32_t booleanLimit){
    if(stream.size()>65536)throw std::invalid_argument("lighting queue size");
    GenericLightingUploadBundleCandidate result;result.floats.selections={0};size_t at=0;bool metadataSeen=false;
    while(at<stream.size()){
        if(stream.size()-at<3)throw std::invalid_argument("lighting queue header");
        const uint32_t opcode=stream[at++],index=stream[at++],count=stream[at++];
        if(opcode!=10&&opcode!=11&&opcode!=12)throw std::invalid_argument("lighting queue opcode");
        if(opcode==10&&metadataSeen)throw std::invalid_argument("lighting queue bundle order");
        const size_t width=opcode==11?1:4;const uint32_t limit=opcode==10?floatLimit:(opcode==11?booleanLimit:integerLimit);
        if(index>limit||count>limit-index||count>(stream.size()-at)/width)throw std::invalid_argument("lighting queue range");
        const size_t words=size_t(count)*width;std::vector<uint32_t> values(stream.begin()+at,stream.begin()+at+words);at+=words;
        if(opcode==10)result.floats.uploads.push_back({false,index,std::move(values)});
        else{metadataSeen=true;result.metadata.push_back({opcode==11?VertexMetadataKindCandidate::Boolean:VertexMetadataKindCandidate::Integer,index,std::move(values),false});}
    }
    return result; // Retained CPU record is not serialized into device commands.
}
