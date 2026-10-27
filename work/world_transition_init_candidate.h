#pragma once
#include "world_transition_parameter_schema_candidate.h"
#include "lightmapped_generic_init_candidate.h"
// Installed WorldVertexTransition_DX9 uses the same helper body as
// LightmappedGeneric, with50 real declared variables and a different mapping.
// Undeclared fields stay absent even if a similarly named dynamic var exists.
inline std::array<int32_t,68>worldTransitionSharedMappingCandidate(){
    std::array<int32_t,68>map;map.fill(-1);
    const auto&source=lightmappedGenericParameterNamesCandidate();const auto&target=worldTransitionParameterNamesCandidate();
    auto equal=[](std::string_view a,std::string_view b){if(a.size()!=b.size())return false;for(size_t k=0;k<a.size();++k){auto lower=[](char c){return c>='A'&&c<='Z'?char(c+32):c;};if(lower(a[k])!=lower(b[k]))return false;}return true;};
    for(size_t i=0;i<source.size();++i)for(size_t j=0;j<target.size();++j)if(equal(source[i],target[j])){map[i]=int32_t(j);break;}
    return map;
}
using WorldTransitionCompletionCandidate=LightmappedSharedCompletionCandidate;
inline bool worldTransitionCompletionCandidate(WorldTransitionCompletionCandidate&out,
        const std::vector<MaterialParameterCompletedValueCandidate>&input,const std::vector<uint32_t>&types,
        const VertexLitInitSettingsCandidate&common,const LightmappedInitSettingsCandidate&s){
    if(types!=worldTransitionParameterTypesCandidate()||input.size()<50)return false;
    return lightmappedSharedCompletionCandidate(out,input,types,common,s,worldTransitionSharedMappingCandidate());
}
