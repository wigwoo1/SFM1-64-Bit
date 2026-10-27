#pragma once
#include "lightmapped_generic_parameter_schema_candidate.h"
#include "lightmapped_shared_parameter_init_candidate.h"
using LightmappedGenericCompletionCandidate=LightmappedSharedCompletionCandidate;
inline bool lightmappedGenericCompletionCandidate(LightmappedGenericCompletionCandidate&out,
        const std::vector<MaterialParameterCompletedValueCandidate>&input,const std::vector<uint32_t>&types,
        const VertexLitInitSettingsCandidate&common,const LightmappedInitSettingsCandidate&s){
    if(types!=lightmappedGenericParameterTypesCandidate()||input.size()<68)return false;
    std::array<int32_t,68>mapping{};for(size_t i=0;i<mapping.size();++i)mapping[i]=int32_t(i);
    return lightmappedSharedCompletionCandidate(out,input,types,common,s,mapping);
}
