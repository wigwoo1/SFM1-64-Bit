#pragma once
#include "generic_lighting_constants_record_candidate.h"
#include "native_shader_command_plan_candidate.h"
#include <cstring>
// Stable owned configuration input. Original routine queries hardware repeatedly;
// runtime mutation/replacement semantics are not modeled here.
inline NativeShaderCommandPlanCandidate genericLightFloatUploadCandidate(
    const GenericLightingConstantsRecordCandidate& record,int32_t configuredCount,
    bool skipUnusedFill,const std::vector<uint32_t>& unusedWords){
    int32_t count;std::memcpy(&count,&record.words[24],4);
    if(count<0||count>4||configuredCount<0||configuredCount>4)throw std::invalid_argument("light upload capacity");
    NativeShaderCommandPlanCandidate plan;plan.selections={0};
    if(count>0)plan.uploads.push_back({false,27,{record.words.begin()+49,record.words.begin()+49+count*20}});
    if(!skipUnusedFill&&count<configuredCount){
        const size_t needed=size_t(configuredCount-count)*20;
        if(unusedWords.size()<needed)throw std::invalid_argument("unused light source bounds");
        plan.uploads.push_back({false,27+uint32_t(count)*5,{unusedWords.begin(),unusedWords.begin()+needed}});
    }
    return plan;
}
