#include "native_d3d9_lighting_metadata_candidate.h"
#include "queued_vertex_metadata_candidate.h"
#include "native_d3d9_lighting_bundle_candidate.h"
#include "queued_metadata_blocks_candidate.h"
#include "queued_lighting_bundle_candidate.h"
#include <iostream>
int main(){try{
    static_assert(sizeof(void*)==8,"x64 required");
    static_assert(sizeof(int)==4&&sizeof(BOOL)==4,"D3D9 register word width");
    GenericLightingConstantsRecordCandidate record;
    auto plan=genericLightingMetadataCandidate(record);
    GenericLightingUploadBundleCandidate lighting;lighting.metadata=plan;
    lighting.floats.uploads.push_back({false,27,{0x3f800000u,0,0x80000000u,0x7fc01234u}});
    auto lightingQueue=queuedLightingBundleCandidate(lighting);
    auto decodedLighting=decodeQueuedLightingBundleCandidate(lightingQueue,256,16,16);
    if(decodedLighting.floats.uploads.size()!=1||decodedLighting.metadata.size()!=2||decodedLighting.floats.uploads[0].index!=27||decodedLighting.floats.uploads[0].words!=lighting.floats.uploads[0].words||decodedLighting.metadata[0].words!=plan[0].words||decodedLighting.metadata[1].words!=plan[1].words)throw std::logic_error("lighting queue roundtrip");
    auto truncatedLighting=lightingQueue;truncatedLighting.pop_back();
    try{decodeQueuedLightingBundleCandidate(truncatedLighting,256,16,16);throw std::logic_error("lighting queue truncation accepted");}catch(const std::invalid_argument&){}
    std::cout<<"combined_lighting_queue_roundtrip=passed truncation_rejected=1\n";
    auto queue=queuedVertexMetadataCandidate(plan[0]);auto booleanQueue=queuedVertexMetadataCandidate(plan[1]);queue.insert(queue.end(),booleanQueue.begin(),booleanQueue.end());
    auto decoded=decodeQueuedVertexMetadataCandidate(queue,16,16);
    if(decoded.size()!=2||decoded[0].kind!=plan[0].kind||decoded[1].kind!=plan[1].kind||decoded[0].words!=plan[0].words||decoded[1].words!=plan[1].words||queue[0]!=12||queue[7]!=11)throw std::logic_error("metadata queue roundtrip");
    for(const auto& bad:std::vector<std::vector<uint32_t>>{{11},{13,0,0},{12,0,1,0},{11,16,1,0}}){
        try{decodeQueuedVertexMetadataCandidate(bad,16,16);throw std::logic_error("invalid queue accepted");}catch(const std::invalid_argument&){}
    }
    std::cout<<"metadata_device_queue_roundtrip=passed malformed_streams_rejected=4\n";
    std::vector<VertexMetadataUploadCandidate> blockCommands(500,{VertexMetadataKindCandidate::Integer,0,std::vector<uint32_t>(16,0x7134a9cbu),false});
    auto blocks=queuedMetadataBlocksCandidate(blockCommands);size_t decodedCount=0;
    if(blocks.size()!=3)throw std::logic_error("metadata block rollover");
    for(const auto& block:blocks){
        if(block.front()!=2||block.back()!=0||block.size()>4096)throw std::logic_error("metadata block envelope");
        auto decodedBlock=decodeQueuedVertexMetadataCandidate({block.begin()+1,block.end()-1},16,16);
        for(const auto& command:decodedBlock)if(command.kind!=VertexMetadataKindCandidate::Integer||command.words!=blockCommands[0].words)throw std::logic_error("metadata block payload");
        decodedCount+=decodedBlock.size();
    }
    if(decodedCount!=500||!queuedMetadataBlocksCandidate({}).empty())throw std::logic_error("metadata block command count");
    try{queuedMetadataBlocksCandidate({{VertexMetadataKindCandidate::Boolean,0,std::vector<uint32_t>(4092),false}});throw std::logic_error("oversized metadata block accepted");}catch(const std::invalid_argument&){}
    std::cout<<"metadata_block_rollover_commands=500 blocks=3 oversized_command_rejected=1\n";
    size_t dispatched=0;
    if(dispatchQueuedMetadataBlocksCandidate(blocks,16,16,[&](const auto& command){if(command.words!=blockCommands[dispatched].words)throw std::logic_error("metadata dispatch order");++dispatched;})!=500||dispatched!=500)throw std::logic_error("metadata block dispatch count");
    auto malformedBlocks=blocks;malformedBlocks.back().back()=7;
    try{dispatchQueuedMetadataBlocksCandidate(malformedBlocks,16,16,[&](const auto&){++dispatched;});throw std::logic_error("invalid block batch accepted");}catch(const std::invalid_argument&){}
    if(dispatched!=500)throw std::logic_error("late malformed block caused callbacks");
    std::cout<<"metadata_block_dispatch=500 late_invalid_block_callbacks=0\n";
    try{nativeD3d9LightingMetadataCandidate(nullptr,plan,16,16);throw std::logic_error("missing device accepted");}catch(const std::runtime_error& e){if(std::string(e.what())!="D3D9 metadata device missing")throw;}
    plan.back().index=16;
    try{nativeD3d9LightingMetadataCandidate(nullptr,plan,16,16);throw std::logic_error("range accepted");}catch(const std::invalid_argument& e){if(std::string(e.what())!="metadata range")throw;}
    try{RetainedNativeD3d9LightingMetadataCandidate cache(nullptr,16,16);throw std::logic_error("missing owner device accepted");}catch(const std::runtime_error& e){if(std::string(e.what())!="retained D3D9 metadata device missing")throw;}
    try{RetainedNativeD3d9LightingBundleCandidate owner(nullptr,256,32,16,16);throw std::logic_error("missing bundle device accepted");}catch(const std::runtime_error& e){if(std::string(e.what())!="retained D3D9 constant device missing")throw;}
    std::cout<<"x64_metadata_adapter_build_and_prevalidation=passed live_device_uploads=unverified\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
